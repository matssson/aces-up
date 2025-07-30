#include <array>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

struct Card {
    std::uint8_t id;
    constexpr Card(const std::uint8_t id_ = 0) : id{id_} {}
    constexpr std::uint8_t suit() const { return id / 13; }
    constexpr std::uint8_t rank() const { return id % 13; }

    friend std::ostream& operator<<(std::ostream& os, const Card& c) {
        static constexpr char RANKS[13]{
            '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
        };
        static constexpr char SUITS[4]{ 'S', 'H', 'D', 'C' };
        return os << RANKS[c.rank()] << SUITS[c.suit()];
    }
};

struct Deck {
    std::array<Card, 52> cards{};
    constexpr Deck() { reset(); }
    constexpr void reset() {
        for (std::uint8_t i = 0; i < 52; ++i) cards[i] = Card{i};
    }

    void shuffle(const std::uint64_t seed) {
        std::mt19937_64 rng;
        reset();
        rng.seed(seed);
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    friend std::ostream& operator<<(std::ostream& os, const Deck& d) {
        for (std::size_t i = 0; i < 52; ++i) {
            os << d.cards[i];
            os << (((i + 1) % 13) ? ' ' : '\n');
        }
        return os;
    }
};

struct Pile {
    std::array<Card, 13> data{};
    std::uint8_t sz = 0;

    bool empty() const { return sz == 0; }
    std::size_t size() const { return sz; }
    const Card& back() const { return data[sz - 1]; }
    void push_back(Card c) { data[sz++] = c; }
    void pop_back() { --sz; }
    const Card& operator[](std::size_t i) const { return data[i]; }
    void clear() noexcept { sz = 0; }
};

struct Move {
    std::int8_t from = -1;
    std::int8_t to = -1;
    constexpr Move() = default;
    constexpr Move(std::int8_t f, std::int8_t t) : from{f}, to{t} {}

    friend std::ostream& operator<<(std::ostream& os, const Move& m) {
        if (m.to != -1) {
            os << "Move from pile " << int(m.from) + 1 << " to pile " << int(m.to) + 1 << ".\n";
        } else if (m.from != -1) {
            os << "Discard from pile " << int(m.from) + 1 << ".\n";
        } else {
            os << "Deal four cards.\n";
        }
        return os;
    }
};

struct MoveBuff {
    Move v[6];
    std::uint8_t sz = 0;

    void clear() { sz = 0; }
    void emplace_back(std::int8_t from, std::int8_t to) {
        v[sz].from = from;
        v[sz].to = to;
        sz++;
    }
    std::size_t size() const { return sz; }
    const Move& operator[](std::size_t i) const { return v[i]; }

    using iterator = Move*;
    iterator begin() noexcept { return v; }
    iterator end() noexcept { return v + sz; }
};

struct Board {
    Deck deck;
    std::array<Pile, 4> piles{};
    std::uint8_t card_idx = 0;

    Board(std::uint64_t seed) { reset(seed); }

    void reset(std::uint64_t seed) {
        for (auto& p : piles) p.clear();
        deck.shuffle(seed);
        card_idx = 0;
    }

    constexpr bool can_deal() const { return card_idx < 52; }
    int score() const {
        if (can_deal()) return 100;
        int score = -4;
        for (auto& p : piles) {
            score += p.size();
        }
        return score;
    }
    void deal_four() {
        for (int i = 0; i < 4; ++i) {
            piles[i].push_back(deck.cards[card_idx++]);
        }
    }
    void move_to_empty(std::int8_t from, std::int8_t to) {
        auto c = piles[from].back();
        piles[from].pop_back();
        piles[to].push_back(c);
    }
    void discard(std::int8_t from) { piles[from].pop_back(); }

    void move(std::int8_t from, std::int8_t to) {
        if (to != -1) {
            move_to_empty(from, to);
        } else if (from != -1) {
            discard(from);
        } else {
            deal_four();
        }
    }

    void legal_moves(MoveBuff& moves) const {
        moves.clear();
        struct Top { std::int8_t pile = -1; Card c{}; };
        Top top_cards[4];
        std::int8_t empty_pile = -1;
        for (std::int8_t i = 0; i < 4; ++i) {
            if (!piles[i].empty()) {
                top_cards[i].pile = i;
                top_cards[i].c = piles[i].back();
            } else {
                empty_pile = i;
            }
        }

        std::uint8_t maxRankBySuit[4] = { 0, 0, 0, 0 };
        std::uint8_t countBySuit[4] = { 0, 0, 0, 0 };
        for (auto& t : top_cards) if (t.pile != -1) {
            maxRankBySuit[t.c.suit()] = std::max(maxRankBySuit[t.c.suit()], t.c.rank());
            ++countBySuit[t.c.suit()];
        }
        for (auto& t : top_cards) if (t.pile != -1) {
            if (countBySuit[t.c.suit()] >= 2 && t.c.rank() < maxRankBySuit[t.c.suit()]) {
                moves.emplace_back(t.pile, -1);
            }
            if (empty_pile != -1 && piles[t.pile].size() > 1) {
                moves.emplace_back(t.pile, empty_pile);
            }
        }

        if (can_deal()) { moves.emplace_back(-1, -1); }
    }

    friend std::ostream& operator<<(std::ostream& os, const Board& b) {
        std::size_t h = 0;
        for (int i = 0; i < 4; ++i) h = std::max(h, b.piles[i].size());
        os << "Board state:\n-----------------\n";
        for (std::size_t row = 0; row < h; ++row) {
            for (int i = 0; i < 4; ++i) {
                if (b.piles[i].size() > row) os << b.piles[i][row] << "   ";
                else os << "     ";
            }
            os << '\n';
        }
        os << "-----------------\n";
        os << "Cards left: " << 52 - int(b.card_idx) << "/52\n";
        os << "Current score: " << b.score() << "\n";
        MoveBuff moves;
        b.legal_moves(moves);
        os << "Legal moves:\n";
        for (auto& m : moves) {
            os << m;
        }
        return os;
    }
};

void solve_score(Board& board, std::vector<Move>& path, std::vector<Move>& best_path, int& best_score, MoveBuff& moves) {
    if (moves.size() == 0) {
        int s = board.score();
        if (s < best_score) {
            best_score = s;
            best_path = path;
        }
        return;
    }
    for (auto& m : moves) {
        Board copy = board;
        copy.move(m.from, m.to);
        path.push_back(m);
        int ctr = 1;
        MoveBuff copy_moves;
        copy.legal_moves(copy_moves);
        for (std::size_t i = 0; i < copy_moves.size(); ++i) {
            if (copy_moves[i].to == -1 && copy_moves[i].from != -1) {
                path.emplace_back(copy_moves[i].from, copy_moves[i].to);
                copy.move(copy_moves[i].from, copy_moves[i].to);
                copy.legal_moves(copy_moves);
                i = -1;
                ctr++;
            }
        }
        solve_score(copy, path, best_path, best_score, copy_moves);
        if (best_score == 0) return;
        path.resize(path.size() - ctr);
    }
}

int solve(std::uint64_t seed, bool print = false) {
    std::vector<Move> best_path;
    std::vector<Move> path;
    Board board(seed);
    best_path.reserve(64);
    path.reserve(64);
    int best_score = 1000;
    MoveBuff moves;
    board.legal_moves(moves);
    solve_score(board, path, best_path, best_score, moves);
    if (print) {
        board.reset(seed);
        for (auto& p : best_path) {
            std::cout << board << "Applying move: " << p << "\n";
            board.move(p.from, p.to);
        }
        std::cout << board << "\n";
    }
    return best_score;
}

int main() {
    int games = 1000;
    int score = 0;
    int finished_games = 0;
    for (int i = 1; i <= games; ++i) {
        std::cout << i << "\n";
        int tc = solve(i, false);
        if (tc == 0) finished_games++;
        score += tc;
    }
    std::cout << finished_games << "\n";
    std::cout << double(score)/double(games) << "\n";
    return 0;
}
