def calc(x : int, y: int, operation):
    if operation == "div" and  y == 0:
        print("Error: cannot divide by zero")
        return

    operations = {
        "add": x + y,
        "sub": x - y,
        "multiply": x*y,
        "div": x/y,
    }

    result = f"{operations[operation]}"
    print(result)
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description= "Provides arthemethic operations"
    )
    parser.add_argument(
        "x", type=int, help="First number"
    )
    parser.add_argument(
        "y", type=int, help="Second number"
    )
    parser.add_argument(
        "operation", choices=["add", "sub", "multiply", "div"], help="operation to perform"
    )
    args = parser.parse_args()
    calc(args.x, args.y, args.operation)