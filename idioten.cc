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
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

constexpr int N_RANKS = 13, N_SUITS = 4, N_PILES = 4, N_ACES = 4;
constexpr int N_CARDS = N_RANKS * N_SUITS;
constexpr int MAX_SCORE = N_CARDS - N_ACES;
constexpr int M_EMPTY = -1;

using TranspositionTable = std::unordered_set<std::string>;

struct Card {
    std::uint8_t packed_id;

    static constexpr char SUITS[N_SUITS] = { 'S', 'H', 'D', 'C' };
    static constexpr char RANKS[N_RANKS] = { '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A' };
    static constexpr std::uint8_t LUT[N_CARDS] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C
    };
    static constexpr unsigned SUIT_SHIFT = 4;
    static constexpr unsigned RANK_MASK = 0x0F;

    constexpr Card(int id = 0) : packed_id{LUT[id]} {}
    constexpr int suit() const { return packed_id >> SUIT_SHIFT; }
    constexpr int rank() const { return packed_id & RANK_MASK; }

    friend std::ostream& operator<<(std::ostream& os, Card c)
    {
        return os << RANKS[c.rank()] << SUITS[c.suit()];
    }
};

struct Deck {
    std::array<Card, N_CARDS> cards{};

    constexpr void reset()
    {
        for (int i = 0; i < N_CARDS; ++i) cards[i] = Card{i};
    }
    void shuffle(std::uint64_t seed)
    {
        reset();
        std::mt19937_64 rng{seed};
        std::shuffle(cards.begin(), cards.end(), rng);
    }
    constexpr Deck() { reset(); }
    Deck(std::uint64_t seed) { shuffle(seed); }

    friend std::ostream& operator<<(std::ostream& os, const Deck& d)
    {
        for (int i = 0; i < N_CARDS; ++i) {
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
    constexpr int size() const { return sz; }
    void push_back(Card c) { data[sz++] = c; }
    void pop_back() { --sz; }
    void clear() { sz = 0; }
    const Card& back() const { return data[sz - 1]; }
    const Card& operator[](int i) const { return data[i]; }
};

struct Move {
    int from;
    int to;

    constexpr Move(int f = M_EMPTY, int t = M_EMPTY) : from{f}, to{t} {}

    constexpr bool is_move_to_empty_pile() const { return to != M_EMPTY; }
    constexpr bool is_discard() const { return to == M_EMPTY && from != M_EMPTY; }
    constexpr bool is_deal_four() const { return to == M_EMPTY && from == M_EMPTY; }

    friend std::ostream& operator<<(std::ostream& os, const Move& m)
    {
        if (m.is_discard()) os << "Discard from pile " << m.from + 1 << ".\n";
        else if (m.is_deal_four()) os << "Deal four cards.\n";
        else os << "Move from pile " << m.from + 1 << " to pile " << m.to + 1 << ".\n";
        return os;
    }
};

struct DFSMoveBuff {
    static constexpr int MAX_MOVES_TO_EMPTY_PLUS_DEAL = 5;
    std::array<Move, MAX_MOVES_TO_EMPTY_PLUS_DEAL> v{};
    std::uint8_t sz = 0;

    void clear() { sz = 0; }
    void emplace_back(int from, int to)
    {
        v[sz].from = from;
        v[sz++].to = to;
    }
    constexpr bool empty() const { return sz == 0; }

    using const_iterator = const Move*;
    const_iterator begin() const { return v.data(); }
    const_iterator end() const { return v.data() + sz; }
};

struct Board {
    std::array<Pile, N_PILES> piles{};
    const Deck deck;
    int card_idx = 0;

    Board(std::uint64_t seed) : deck{seed} {}

    void reset()
    {
        for (auto& p : piles) p.clear();
        card_idx = 0;
    }

    constexpr bool can_deal() const { return card_idx < N_CARDS; }
    int score() const
    {
        if (can_deal()) return MAX_SCORE + 1;
        int score = -N_ACES;
        for (const auto& p : piles) score += p.size();
        return score;
    }
    void move_to_empty_pile(int from, int to)
    {
        const auto c = piles[from].back();
        piles[to].push_back(c);
        piles[from].pop_back();
    }
    void discard(int from) { piles[from].pop_back(); }
    void deal_four()
    {
        for (int i = 0; i < N_PILES; ++i) {
            piles[i].push_back(deck.cards[card_idx++]);
        }
    }
    void apply_move(const Move& move)
    {
        if (move.to != M_EMPTY) move_to_empty_pile(move.from, move.to);
        else if (move.from != M_EMPTY) discard(move.from);
        else deal_four();
    }

    std::vector<Move> legal_moves() const
    {
        std::vector<Move> moves{};
        int max_rank_by_suit[N_SUITS] = { -1, -1, -1, -1 };
        for (int i = 0; i < N_PILES; ++i) if (!piles[i].empty()) {
            const auto c = piles[i].back();
            max_rank_by_suit[c.suit()] = std::max(max_rank_by_suit[c.suit()], c.rank());
        }
        for (int from = 0; from < N_PILES; ++from) if (!piles[from].empty()) {
            const auto c = piles[from].back();
            if (c.rank() < max_rank_by_suit[c.suit()]) {
                moves.emplace_back(from, M_EMPTY);
            }
            if (piles[from].size() > 1) {
                for (int to = 0; to < N_PILES; ++to) if (piles[to].empty()) {
                    moves.emplace_back(from, to);
                }
            }
        }
        if (can_deal()) moves.emplace_back(M_EMPTY, M_EMPTY);
        return moves;
    }

    DFSMoveBuff pruned_legal_non_discards() const
    {
        DFSMoveBuff moves{};
        for (int from = 0; from < N_PILES; ++from) if (piles[from].size() > 1) {
            for (int to = 0; to < N_PILES; ++to) if (piles[to].empty()) {
                const auto size = piles[from].size();
                const auto c1 = piles[from][size - 1];
                const auto c2 = piles[from][size - 2];
                if (c1.suit() == c2.suit() && c1.rank() < c2.rank()) {
                    moves.clear();
                    moves.emplace_back(from, to);
                    return moves;
                }
                moves.emplace_back(from, to);
            }
        }
        if (can_deal()) moves.emplace_back(M_EMPTY, M_EMPTY);
        return moves;
    }

    template<bool TRACE>
    void discard_all(std::vector<Move>* path)
    {
        int max_rank_by_suit[N_SUITS] = { -1, -1, -1, -1 };
        int loops = 2;
        while (loops-- > 0) {
            for (int i = 0; i < N_PILES; ++i) {
                if (piles[i].empty()) continue;
                const auto c = piles[i].back();
                if (c.rank() < max_rank_by_suit[c.suit()]) {
                    if constexpr (TRACE) path->emplace_back(i, M_EMPTY);
                    discard(i);
                    loops = 2;
                    break;
                } else { max_rank_by_suit[c.suit()] = c.rank(); }
            }
        }
    }

    std::string serialize() const
    {
        std::string s;
        s.resize(53);
        s[0] = static_cast<char>(card_idx);
        int pos = 1;
        for (const auto& p : piles) {
            for (int i = 0; i < N_CARDS/N_PILES; ++i) {
                if (i < p.size()) s[pos++] = static_cast<char>(p[i].packed_id);
                else s[pos++] = '_'; // Outside the range of packed_id
            }
        }
        return s;
    }

    friend std::ostream& operator<<(std::ostream& os, const Board& b)
    {
        int h = 0;
        for (int i = 0; i < N_PILES; ++i) h = std::max(h, b.piles[i].size());
        os << "-----------------\n";
        for (int row = 0; row < h; ++row) {
            for (int i = 0; i < N_PILES; ++i) {
                if (b.piles[i].size() > row) os << b.piles[i][row] << "   ";
                else os << "     ";
            }
            os << '\n';
        }
        if (h == 0) os << "\n";
        os << "-----------------\n";
        os << "Cards left: " << N_CARDS - b.card_idx << "/" << N_CARDS << "\n";
        const auto moves = b.legal_moves();
        if (moves.empty()) os << "No moves left, final score: " << b.score() << "\n";
        else os << "Legal moves:\n";
        for (const auto& m : moves) os << "    " << m;
        return os;
    }
};

template<bool TRACE>
void solve_dfs(const Board& board,
               int& best_score,
               TranspositionTable* tt,
               std::vector<Move>* path,
               std::vector<Move>* best_path)
{
    const auto avail_moves = board.pruned_legal_non_discards();
    if (avail_moves.empty()) {
        const int s = board.score();
        if (s < best_score) {
            if constexpr (TRACE) *best_path = *path;
            best_score = s;
        }
        return;
    }
    auto [_, new_board] = tt->emplace(board.serialize());
    if (!new_board) return;
    for (const auto& m : avail_moves) {
        Board copy = board;
        if constexpr (TRACE) {
            const auto checkpoint = path->size();
            path->push_back(m);
            copy.apply_move(m);
            copy.discard_all<TRACE>(path);
            solve_dfs<TRACE>(copy, best_score, tt, path, best_path);
            path->resize(checkpoint);
        } else {
            copy.apply_move(m);
            copy.discard_all<TRACE>(path);
            solve_dfs<TRACE>(copy, best_score, tt, path, best_path);
        }
        if (best_score == 0) return;
    }
}

template<bool TRACE>
int solve(std::uint64_t seed, TranspositionTable* tt = nullptr)
{
    Board board{seed};
    int best_score = 1000;
    if constexpr (TRACE) {
        TranspositionTable temp_tt{};
        std::vector<Move> path, best_path;
        path.reserve(80); best_path.reserve(80);
        solve_dfs<TRACE>(board, best_score, &temp_tt, &path, &best_path);

        int move = 0;
        for (const auto& p : best_path) {
            std::cout << "Board state (" << move++ << " moves)\n" << board;
            std::cout << "Applying move: " << p << "\n";
            board.apply_move(p);
        }
        std::cout << "Board state (" << best_path.size() << " moves)\n" << board;
    } else {
        tt->clear();
        solve_dfs<TRACE>(board, best_score, tt, nullptr, nullptr);
    }
    return best_score;
}

int main()
{
    constexpr int n_games = 1'000'000;
    constexpr int master_seed = 42;
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
            TranspositionTable tt{};
            for (int i = start; i < end; ++i) scores[i] = solve<false>(seeds[i], &tt);
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    {
        std::ofstream raw("build/scores_raw.csv");
        raw << "game,score,seed\n";
        for (int i = 0; i < n_games; ++i) if (scores[i] >= 40) {
            raw << i + 1 << ',' << scores[i] << ',' << seeds[i] << '\n';
        }

        std::array<int, MAX_SCORE + 1> freq{};
        for (int s : scores) ++freq[s];
        std::ofstream counts("data/score_counts.csv");
        counts << "score,count\n";
        for (int s = 0; s <= MAX_SCORE; ++s) counts << s << ',' << freq[s] << '\n';
    }
    return 0;
}
