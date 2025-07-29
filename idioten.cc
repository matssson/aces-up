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

    void shuffle(const std::uint64_t seed = 0) {
        std::mt19937_64 rng;
        if (seed) {
            reset();
            rng.seed(seed);
        } else {
            rng.seed(static_cast<std::mt19937_64::result_type>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        }
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
        os << "From " << int(m.from) << " to " << int(m.to) << ".\n";
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

    Board(std::uint64_t seed = 0) { reset(seed); }

    void reset(std::uint64_t seed = 0) {
        for (auto& p : piles) p.clear();
        deck.reset();
        deck.shuffle(seed);
        card_idx = 0;
    }

    constexpr bool can_deal() const { return card_idx < 52; }
    int score() const {
        int score = 52 - card_idx - 4;
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
        os << "-----------------\n";
        for (std::size_t row = 0; row < h; ++row) {
            for (int i = 0; i < 4; ++i) {
                if (b.piles[i].size() > row) os << b.piles[i][row] << "   ";
                else os << "     ";
            }
            os << '\n';
        }
        os << "-----------------\n";
        os << "Card index: " << static_cast<int>(b.card_idx) << "/52\n";
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

int solve_score(Board board) {
    MoveBuff moves;
    board.legal_moves(moves);
    if (moves.size() == 0) {
        return board.score();
    }
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (moves[i].to == -1 && moves[i].from != -1) {
            board.move(moves[i].from, moves[i].to);
            board.legal_moves(moves);
            i = -1;
        }
    }
    int min_score = board.score();
    for (auto& m : moves) {
        Board copy = board;
        copy.move(m.from, m.to);
        min_score = std::min(min_score, solve_score(copy));
        if (min_score == 0) return 0;
    }
    return min_score;
}

int main() {
    int games = 1000;
    int score = 0;
    int finished_games = 0;
    for (int i = 1; i <= games; ++i) {
        int tc = solve_score(Board(i));
        std::cout << i << "\n";
        if (tc == 0) finished_games++;
        score += tc;
    }
    std::cout << finished_games << "\n";
    std::cout << double(score)/double(games) << "\n";
    return 0;
}
