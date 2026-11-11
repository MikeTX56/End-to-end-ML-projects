import sys
import random
from enum import Enum


def rps(name="playerOne"):
    game_count = 0
    player_wins = 0
    computer_wins = 0

    def play_rps():
        nonlocal name
        nonlocal player_wins
        nonlocal computer_wins

        class RPS(Enum):
            ROCK = 1
            PAPER = 2
            SCISSOR = 3

        player_choice = input(f"\n {name} please enter....\n1 for Rock\n2 for Paper\n3 for Scissor\n\n")
        if player_choice not in ["1", "2", "3"]:
            print("you must enter number between 1, 2 or 3")
            return play_rps()
        player = int(player_choice)

        computer_choice = random.choice("123")
        computer = int(computer_choice)
        print(f"{name}, you chose {str(RPS(player)).replace("RPS.",'').title() } ")
        print(f"\ncomputer chose{ str(RPS(computer)).replace("RPS.", '').title()}.\n")

        def decide_winner(player, computer):
            nonlocal name
            nonlocal player_wins
            nonlocal computer_wins
            if player == 1 and computer == 3:
                player_wins +=1
                print(f"{name}, you wins")
            elif player == 3 and computer == 2:
                player_wins +=1
                print(f"{name}, you wins")
            elif player == 2 and computer == 1:
                player_wins +=1
                print(f"{name}, you wins")
            elif player == computer:
                print("its tie game")
            else:
                computer_wins +=1
                print(f"computer wins")
        game_result = decide_winner(player, computer)
        print(game_result)
        nonlocal game_count
        game_count += 1

        print(f"\n Game count: {game_count}")
        print(f"\n {name}'s wins: {player_wins}")
        print(f"\n Computer wins: {computer_wins}")

        print(f"Play again?")
        while True:
            play_again = input("\n Do you want to play again? \n Y for yes \n Q for quit\n\n")
            if play_again.lower() not in ["y", "q"]:
                continue
            else:
                break
        if play_again == "y":
            return play_rps()
        else:
            print("Celebrate")
            play_again = False
            sys.exit(f"Bye! {name}")
    return play_rps()



if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description = "provides a personalized game experiance."

    )

    parser.add_argument(
        "-n", "--name", metavar = "name",
        required = True, help = "The name of the person playing the game."
    )
    args = parser.parse_args()
    rock_paper_scissors = rps(args.name) 
    rock_paper_scissors()