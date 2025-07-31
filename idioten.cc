#include <array>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

constexpr int N_RANKS {13};
constexpr int N_SUITS {4};
constexpr int N_PILES {4};
constexpr int N_CARDS {N_RANKS * N_SUITS};
constexpr char RANKS[N_RANKS]{ '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A' };
constexpr char SUITS[N_SUITS]{ 'S', 'H', 'D', 'C' };
constexpr std::int8_t M_EMPTY {-1};

struct Card {
    std::uint8_t id;
    constexpr Card(const std::uint8_t id_ = 0) : id{id_} {}
    constexpr std::uint8_t suit() const { return id / N_RANKS; }
    constexpr std::uint8_t rank() const { return id % N_RANKS; }
    friend std::ostream& operator<<(std::ostream& os, const Card& c) {
        return os << RANKS[c.rank()] << SUITS[c.suit()];
    }
};

struct Deck {
    std::array<Card, N_CARDS> cards{};
    constexpr Deck() { reset(); }
    constexpr void reset() {
        for (std::uint8_t i = 0; i < N_CARDS; ++i) cards[i] = Card{i};
    }

    void shuffle(const std::uint64_t seed) {
        std::mt19937_64 rng;
        reset();
        rng.seed(seed);
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    friend std::ostream& operator<<(std::ostream& os, const Deck& d) {
        for (std::size_t i = 0; i < N_CARDS; ++i) {
            os << d.cards[i];
            os << (((i + 1) % N_RANKS) ? ' ' : '\n');
        }
        return os;
    }
};

struct Pile {
    std::array<Card, N_RANKS> data{};
    std::uint8_t sz = 0;

    bool empty() const { return sz == 0; }
    std::size_t size() const { return sz; }
    const Card& back() const { return data[sz - 1]; }
    void push_back(Card c) { data[sz++] = c; }
    void pop_back() { --sz; }
    const Card& operator[](std::size_t i) const { return data[i]; }
    void clear() { sz = 0; }
};

struct Move {
    std::int8_t from = M_EMPTY;
    std::int8_t to = M_EMPTY;
    constexpr Move() = default;
    constexpr Move(std::int8_t f, std::int8_t t) : from{f}, to{t} {}

    friend std::ostream& operator<<(std::ostream& os, const Move& m) {
        if (m.to != M_EMPTY) {
            os << "Move from pile " << int(m.from) + 1 << " to pile " << int(m.to) + 1 << ".\n";
        } else if (m.from != M_EMPTY) {
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
    iterator begin() { return v; }
    iterator end() { return v + sz; }
};

struct Board {
    Deck deck;
    std::array<Pile, N_PILES> piles{};
    std::uint8_t card_idx = 0;

    Board(std::uint64_t seed) { reset(seed); }

    void reset(std::uint64_t seed) {
        for (auto& p : piles) p.clear();
        deck.shuffle(seed);
        card_idx = 0;
    }

    constexpr bool can_deal() const { return card_idx < N_CARDS; }
    int score() const {
        if (can_deal()) return 100;
        int score = -4;
        for (const auto& p : piles) {
            score += p.size();
        }
        return score;
    }
    void deal_four() {
        for (int i = 0; i < N_PILES; ++i) {
            piles[i].push_back(deck.cards[card_idx++]);
        }
    }
    void move_to_empty(std::int8_t from, std::int8_t to) {
        const auto c = piles[from].back();
        piles[to].push_back(c);
        piles[from].pop_back();
    }
    void discard(std::int8_t from) { piles[from].pop_back(); }

    void move(std::int8_t from, std::int8_t to) {
        if (to != M_EMPTY) {
            move_to_empty(from, to);
        } else if (from != M_EMPTY) {
            discard(from);
        } else {
            deal_four();
        }
    }

    void legal_moves(MoveBuff& moves) const {
        moves.clear();
        struct Top { std::int8_t pile = M_EMPTY; Card c{}; };
        Top top_cards[N_PILES];
        std::int8_t empty_pile = M_EMPTY;
        for (std::int8_t i = 0; i < N_PILES; ++i) {
            if (!piles[i].empty()) {
                top_cards[i].pile = i;
                top_cards[i].c = piles[i].back();
            } else {
                empty_pile = i;
            }
        }

        std::uint8_t maxRankBySuit[N_SUITS] = { 0, 0, 0, 0 };
        std::uint8_t countBySuit[N_SUITS] = { 0, 0, 0, 0 };
        for (const auto& t : top_cards) if (t.pile != M_EMPTY) {
            maxRankBySuit[t.c.suit()] = std::max(maxRankBySuit[t.c.suit()], t.c.rank());
            ++countBySuit[t.c.suit()];
        }
        for (const auto& t : top_cards) if (t.pile != M_EMPTY) {
            if (countBySuit[t.c.suit()] >= 2 && t.c.rank() < maxRankBySuit[t.c.suit()]) {
                moves.emplace_back(t.pile, M_EMPTY);
            }
            if (empty_pile != M_EMPTY && piles[t.pile].size() > 1) {
                moves.emplace_back(t.pile, empty_pile);
            }
        }

        if (can_deal()) { moves.emplace_back(M_EMPTY, M_EMPTY); }
    }

    friend std::ostream& operator<<(std::ostream& os, const Board& b) {
        std::size_t h = 0;
        for (int i = 0; i < N_PILES; ++i) h = std::max(h, b.piles[i].size());
        os << "Board state:\n-----------------\n";
        for (std::size_t row = 0; row < h; ++row) {
            for (int i = 0; i < N_PILES; ++i) {
                if (b.piles[i].size() > row) os << b.piles[i][row] << "   ";
                else os << "     ";
            }
            os << '\n';
        }
        os << "-----------------\n";
        os << "Cards left: " << N_CARDS - int(b.card_idx) << "/" << N_CARDS << "\n";
        os << "Current score: " << b.score() << "\n";
        MoveBuff moves;
        b.legal_moves(moves);
        os << "Legal moves:\n";
        for (const auto& m : moves) {
            os << m;
        }
        return os;
    }
};

void solve_score(Board& board, std::vector<Move>& path, std::vector<Move>& best_path, int& best_score, MoveBuff& moves) {
    if (moves.size() == 0) {
        const int s = board.score();
        if (s < best_score) {
            best_score = s;
            best_path = path;
        }
        return;
    }
    for (const auto& m : moves) {
        Board copy = board;
        copy.move(m.from, m.to);
        path.push_back(m);
        int ctr = 1;
        MoveBuff copy_moves;
        copy.legal_moves(copy_moves);
        for (std::size_t i = 0; i < copy_moves.size(); ++i) {
            if (copy_moves[i].to == M_EMPTY && copy_moves[i].from != M_EMPTY) {
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
        for (const auto& p : best_path) {
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
