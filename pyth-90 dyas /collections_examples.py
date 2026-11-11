# from collections import Counter, namedtuple, defaultdict, deque, OrderedDict
# # Counter container that stores elements as its dictionary keys and their counts as dict values
# a = "aaaaaaabbbbbccc"
# my_counter = Counter(a)
# print(my_counter.keys())
# print(my_counter.values())
# print(my_counter.most_common(2)[0][0])
# print(list(my_counter.elements()))

# #namedtuple usd to create simple light weight object type
# point = namedtuple("poiny", "x,y")
# pt = point(1,5)
# print(pt)
# print(pt.x, pt.y)

# #remember the order where the items inserted
# Ordered = OrderedDict()
# # defaultdict have default value if the key is not set yet

# d = defaultdict(int)
# d['a'] = 1
# d['b'] = 2
# print(d['c'])

# #deque double ended que add and helps to remove elements from both ends
# c = deque()
# c.append(1)
# c.append(2)

# c.appendleft(3)
# print(c)