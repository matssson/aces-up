/*
 * Copyright (c) 2025 Matsson <contact@matsson.org>
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

constexpr int N_RANKS = 13;
constexpr int N_SUITS = 4;
constexpr int N_PILES = 4;
constexpr int N_ACES = 4;
constexpr int N_CARDS = N_RANKS * N_SUITS;
constexpr char RANKS[N_RANKS]{ '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A' };
constexpr char SUITS[N_SUITS]{ 'S', 'H', 'D', 'C' };
constexpr std::int8_t M_EMPTY = -1;

struct Card {
    std::uint8_t id;
    constexpr Card(std::uint8_t id_ = 0) : id{id_} {}
    constexpr std::uint8_t suit() const { return id / N_RANKS; }
    constexpr std::uint8_t rank() const { return id % N_RANKS; }
    friend std::ostream& operator<<(std::ostream& os, Card c) {
        return os << RANKS[c.rank()] << SUITS[c.suit()];
    }
};

struct Deck {
    std::array<Card, N_CARDS> cards{};
    constexpr Deck() { reset(); }
    Deck(std::uint64_t seed) { shuffle(seed); }

    constexpr void reset() {
        for (std::uint8_t i = 0; i < N_CARDS; ++i) cards[i] = Card{i};
    }

    void shuffle(std::uint64_t seed) {
        reset();
        std::mt19937_64 rng{seed};
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
    std::array<Card, N_CARDS/N_PILES> data{};
    std::uint8_t sz = 0;

    constexpr bool empty() const { return sz == 0; }
    constexpr std::size_t size() const { return sz; }
    const Card& back() const { return data[sz - 1]; }
    void push_back(Card c) { data[sz++] = c; }
    void pop_back() { --sz; }
    const Card& operator[](std::size_t i) const { return data[i]; }
    void clear() { sz = 0; }
};

struct Move {
    std::int8_t from;
    std::int8_t to;
    constexpr Move() : from{M_EMPTY}, to{M_EMPTY} {}
    constexpr Move(std::int8_t f, std::int8_t t) : from{f}, to{t} {}

    constexpr bool is_move_to_empty_pile() const { return to != M_EMPTY; }
    constexpr bool is_discard() const { return to == M_EMPTY && from != M_EMPTY; }
    constexpr bool is_deal_four() const { return to == M_EMPTY && from == M_EMPTY; }

    friend std::ostream& operator<<(std::ostream& os, const Move& m) {
        if (m.is_discard()) os << "Discard from pile " << m.from + 1 << ".\n";
        else if (m.is_deal_four()) os << "Deal four cards.\n";
        else os << "Move from pile " << m.from + 1 << " to pile " << m.to + 1 << ".\n";
        return os;
    }
};

struct MoveBuff {
    std::array<Move, 6> v{};
    std::uint8_t sz = 0;

    void clear() { sz = 0; }
    void emplace_back(std::int8_t from, std::int8_t to) {
        v[sz].from = from;
        v[sz].to = to;
        sz++;
    }
    constexpr std::size_t size() const { return sz; }
    const Move& operator[](std::size_t i) const { return v[i]; }
    
    using const_iterator = const Move*;
    const_iterator begin() const { return v.data(); }
    const_iterator end() const { return v.data() + sz; }
};

struct Board {
    const Deck deck;
    std::array<Pile, N_PILES> piles{};
    std::uint8_t card_idx = 0;

    Board(std::uint64_t seed) : deck{seed} {}

    void reset() {
        for (auto& p : piles) p.clear();
        card_idx = 0;
    }

    constexpr bool can_deal() const { return card_idx < N_CARDS; }
    int score() const {
        if (can_deal()) return 100;
        int score = -4;
        for (const auto& p : piles) score += p.size();
        return score;
    }

    void move_to_empty_pile(std::int8_t from, std::int8_t to) {
        const auto c = piles[from].back();
        piles[to].push_back(c);
        piles[from].pop_back();
    }
    void discard(std::int8_t from) { piles[from].pop_back(); }
    void deal_four() {
        for (int i = 0; i < N_PILES; ++i) {
            piles[i].push_back(deck.cards[card_idx++]);
        }
    }

    void apply_move(const Move& move) {
        if (move.to != M_EMPTY) move_to_empty_pile(move.from, move.to);
        else if (move.from != M_EMPTY) discard(move.from);
        else deal_four();
    }

    void legal_moves(MoveBuff& moves) const {
        moves.clear();
        struct Top { std::int8_t pile = M_EMPTY; Card c{}; };
        std::array<Top, N_PILES> top_cards{};
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
        os << "-----------------\n";
        for (std::size_t row = 0; row < h; ++row) {
            for (int i = 0; i < N_PILES; ++i) {
                if (b.piles[i].size() > row) os << b.piles[i][row] << "   ";
                else os << "     ";
            }
            os << '\n';
        }
        if (h == 0) os << "\n";
        os << "-----------------\n";
        os << "Cards left: " << N_CARDS - b.card_idx << "/" << N_CARDS << "\n";
        MoveBuff moves;
        b.legal_moves(moves);
        if (moves.size() == 0) os << "No moves left, final score: " << b.score() << "\n";
        else os << "Legal moves:\n";
        for (const auto& m : moves) os << "    " << m;
        return os;
    }
};

void solve_score(const Board& board, const MoveBuff& avail_moves, int& best_score,
                 std::vector<Move>& path, std::vector<Move>& best_path) {
    if (avail_moves.size() == 0) {
        const int s = board.score();
        if (s < best_score) {
            best_score = s;
            best_path = path;
        }
        return;
    }
    for (const auto& m : avail_moves) {
        Board copy = board;
        copy.apply_move(m);
        path.push_back(m);
        MoveBuff copy_moves;
        int ctr = 1;
        bool has_discarded = true;
        while (has_discarded) {
            has_discarded = false;
            copy.legal_moves(copy_moves);
            for (std::size_t i = 0; i < copy_moves.size(); ++i) {
                if (copy_moves[i].is_discard()) {
                    path.push_back(copy_moves[i]);
                    copy.apply_move(copy_moves[i]);
                    has_discarded = true;
                    ctr++;
                }
            }
        }
        solve_score(copy, copy_moves, best_score, path, best_path);
        if (best_score == 0) return;
        path.resize(path.size() - ctr);
    }
}

int solve(std::uint64_t seed, bool print = false) {
    std::vector<Move> best_path;
    std::vector<Move> path;
    best_path.reserve(80);
    path.reserve(80);
    Board board(seed);
    int best_score = 1000;
    MoveBuff moves;
    board.legal_moves(moves);
    solve_score(board, moves, best_score, path, best_path);
    if (print) {
        board.reset();
        for (std::size_t i = 0; i < best_path.size(); ++i) {
            auto p = best_path[i];
            std::cout << "Board state (" << i << " moves)\n" << board << "Applying move: " << p << "\n";
            board.apply_move(p);
        }
        std::cout << "Board state (" << best_path.size() << " moves)\n" << board;
    }
    return best_score;
}

int main() {
    const int n_games = 1'000'000;
    const int master_seed = 42;
    std::vector<int> scores(n_games);
    std::mt19937_64 seed_rng{master_seed};
    std::vector<std::uint64_t> seeds(n_games);
    std::generate(seeds.begin(), seeds.end(), [&seed_rng] { return seed_rng(); });

    const int n_threads = std::max(1u, std::thread::hardware_concurrency());
    const int chunk = (n_games + n_threads - 1) / n_threads;
    std::vector<std::thread> threads;

    for (int t = 0; t < n_threads; ++t) {
        const int start = t * chunk;
        if (start >= n_games) break;
        const int end = std::min(n_games, start + chunk);

        threads.emplace_back([&scores, &seeds, start, end] {
            for (int i = start; i < end; ++i) scores[i] = solve(seeds[i]);
        });
    }
    for (auto& t : threads) t.join();
    const int max_score = N_CARDS - N_ACES;
    {
        std::ofstream raw("build/scores_raw.csv");
        raw << "game,score,seed\n";
        for (int i = 0; i < n_games; ++i) raw << i + 1 << ',' << scores[i] << ',' << seeds[i] << '\n';

        std::array<int, max_score + 1> freq{};
        for (int s : scores) ++freq[s];
        std::ofstream counts("data/score_counts.csv");
        counts << "score,count\n";
        for (int s = 0; s <= max_score; ++s) counts << s << ',' << freq[s] << '\n';
    }
    return 0;
}
