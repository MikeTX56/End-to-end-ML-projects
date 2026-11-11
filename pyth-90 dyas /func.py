def sum(num1, num2):
    if (type(num1) is not int or type(num2) is not int):
        return
    return num1 + num2 
total = sum(3, "r")
print(total)


def multiple_items(*args):
    print(args)
    print(type(args))
multiple_items("mike", "sarah")


def mult_named_items(**kwargs):
    print(kwargs)
    print(type(kwargs))
mult_named_items(first ="mike", second= "sarah")