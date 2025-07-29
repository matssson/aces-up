#include <array>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>

enum class Rank : std::uint8_t {
    Ace, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King
};
enum class Suit : std::uint8_t { Spades, Hearts, Diamonds, Clubs };

struct Card {
    std::uint8_t id;
    constexpr Card(const std::uint8_t id = 0) : id{id} {}
    constexpr Suit suit() const { return static_cast<Suit>(id / 13); }
    constexpr Rank rank() const { return static_cast<Rank>(id % 13); }

    friend std::ostream& operator<<(std::ostream& os, const Card& c) {
        static constexpr char RANKS[13]{
            'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'
        };
        static constexpr char SUITS[4]{ 'S', 'H', 'D', 'C' };
        const auto r = static_cast<unsigned>(c.rank());
        const auto s = static_cast<unsigned>(c.suit());
        return os << RANKS[r] << SUITS[s];
    }
};

struct Deck {
    std::array<std::uint8_t, 52> ids{};
    constexpr Deck() { reset(); }
    constexpr void reset() {
        for (std::uint8_t i = 0; i < 52; ++i) ids[i] = i;
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
        std::shuffle(ids.begin(), ids.end(), rng);
    }

    friend std::ostream& operator<<(std::ostream& os, const Deck& d) {
        for (std::size_t i = 0; i < d.ids.size(); ++i) {
            os << Card{d.ids[i]};
            os << (((i + 1) % 13) ? ' ' : '\n');
        }
        return os;
    }
};

int main() {
    Deck d;
    std::cout << "Ordered deck:\n" << d << '\n';
    d.shuffle();
    std::cout << "Shuffled deck:\n" << d << '\n';
    d.shuffle(42);
    std::cout << "Shuffled deck:\n" << d << '\n';
    return 0;
}
