import sys
import random
from enum import Enum

def rps():
    game_count = 0 
    player_wins = 0
    python_wins = 0


    def play_rps():
        nonlocal player_wins
        nonlocal python_wins
        class RPS(Enum):
            ROCK = 1
            PAPER = 2
            SCISSORS = 3


        playerchoice = input("\nEnter...\n1 for rock,\n2 for paper, \n3 for scissors:\n\n")
        if playerchoice not in ["1", "2", "3"]:
            print("you must enter 1,2, or 3.")
            return play_rps()
        player = int(playerchoice)

        computerchoice = random.choice("123")

        computer = int(computerchoice)

        print("\nyou chose " + str(RPS(player)).replace('RPS.', '') + ".")
        print("Python chose " + str(RPS(computer)).replace('RPS.', '') + ".\n")

        def decide_winner(player, computer):
            nonlocal player_wins
            nonlocal python_wins
            if player == 1 and computerchoice == 3 :
                player_wins += 1
                return "You win!"
            elif player == 2 and computerchoice == 1 :
                player_wins += 1
                return "You win!"
            elif player == 3 and computerchoice == 2 :
                player_wins += 1
                return "You win!"
            elif player == computerchoice:
                return "Tie game!"
            else:
                python_wins += 1
                return "Python wins!"
        game_result = decide_winner(player, computer)
        print(game_result)
        nonlocal game_count
        game_count += 1

        print("\nGame count: " + str(game_count))
        print("\nPlayer wins: " + str(player_wins))
        print("\nPyhton wins: " + str(python_wins))
        print("\nplay again?")

        while True:
            playagain = input("\nplay again? \n Y for Yes or \n Q for Quit\n\n")
            if playagain.lower()not in ['y', 'q']:
                continue
            else:
                break

        if playagain.lower() == "y":
            return play_rps()
        else:
            print("celebrate")
            playagain = False
            sys.exit("Bye!")
    return play_rps
play = rps()
play()