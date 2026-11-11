def printPicnic(itemsDict, leftwidth, rightwidth):
    print("PICNIC ITEMS".center(leftwidth + rightwidth, "-"))
    for k, v in itemsDict.items():
        print(k.ljust(leftwidth, '.') + str(v).rjust(rightwidth))
picnicitems = {"snadwiches": 4, "apples": 12, "cups": 4, "cookiies": 8000}
printPicnic(picnicitems, 11, 5)
printPicnic(picnicitems, 13, 11)
