# Aces Up Solitaire Solver
**Brute‑force search and statistics for the solitaire game *Aces Up* (a.k.a. *Idiot's Delight*).**

```text
Number of games analyzed: 1,000,000,000

Win rate: 23.688% ± 0.005% (Clopper-Pearson w/ 99.99% CI)
```

## Rules

Four tableau piles are dealt in batches of four. Whenever two top cards share a suit, discard the lower rank. Any exposed card that sits on top of another card may be moved to an empty pile. Keep dealing cards until there are no more cards in the deck.

The game is scored from 0 to 48 depending on how many non-ace cards are left when all legal moves have been exhausted, with a score of zero counting as a win.

## Solver

The solver uses a depth‑first brute-force search which explores every possible move sequence and records the lowest possible score.

This can be thought of as the *thoughtful* variant of the game, where the entire deck order is known, and the player can make the best choice on which exposed card to move to which empty pile, and (mainly) when to keep a pile empty, before the next deal.

When we have an empty pile and a card sitting on top of another card of the same suit with higher rank, the branching factor reduces to 1 since we can always move it to the empty pile and discard it, keeping the pile empty.

## Results

I hadn't found any previous results on the winnability of Aces Up, except for the Wikipedia quote that ["Winning chances with good play are about 1 in 43"](https://en.wikipedia.org/wiki/Aces_Up).

For any given shuffle however, the solvability is a lot higher, namely 23.688% ± 0.005% using Clopper-Pearson (exact) with a 99.99% confidence interval (23.683%&ndash;23.693%).

Running the search for n = 1,000,000 games takes under 40 seconds on my Macbook, and the worst score found was 42 (with seed = 1635014440093633329). The worst score for n = 1,000,000,000 games, taking 10.5 hours to run, was 46 (with seed = 15888093322918992564).

![Distribution of scores (n = 1,000,000,000)](data/score_distribution.png)
*Figure&nbsp;1. Distribution of scores (n = 1,000,000,000).*

Note that a score of 3 is less likely than a score of 4. Other than that the distribution trends downwards, with a score of 0 being the most common. See [score_counts.csv](/data/score_counts.csv) for details.

The median score is 3, with mean = 5.545 (SD = 6.227).

To achieve a score of 48, we need a shuffle where every four card draw is a rainbow. The likelihood of this happening is  $\frac{(13!)^4 (4!)^{13}}{52!}$, and a search for such a seed reveals one at 66102677243.

For example runs see [/examples](/examples).

## License

This project is licensed under the MIT License - see [`LICENSE`](LICENSE).
