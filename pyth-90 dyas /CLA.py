import click

@click.command()
def hello(name, lang):
    greetings = {
        "English": "Hello",
        "Spanish": "Holla",
        "German": "Hallo",
    }
    msg = f"{greetings[lang]} {name}"
    print(msg)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description = "provides a personal greeting."

    )

    parser.add_argument(
        "-n", "--name", metavar = "name",
        required = True, help = "The name of the person to greet."
    )
    parser.add_argument(
        "-l", "--lang", metavar="langage",
        required=True, choices=["English", "Spanish", "German"],
        help="The language of the greeting."
    )
    args = parser.parse_args()
    
    hello(args.name, args.lang)