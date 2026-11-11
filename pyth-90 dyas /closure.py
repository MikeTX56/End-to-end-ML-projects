def parent_funct(person):
    coins = 3

    def play_game():
        nonlocal coins
        coins -= 1

        if coins > 1:
            print("\n" + person + "has " + str(coins) + " coins left.")
        elif coins == 1 :
            print("\n" + person + "has " + str(coins) + " coins left.")
        else:
            print("\n" + person + " is out of coins.  ")
    return play_game

tommy = parent_funct("Tommy")
jenny = parent_funct("Jenny")

tommy()
tommy()

jenny()
tommy()

 