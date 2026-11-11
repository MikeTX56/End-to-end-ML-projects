from itertools import product, permutations, combinations, accumulate, groupby
#accumulate return accumulated sum
import operator
a = [1,5,2,3,4]
acc = accumulate(a, func =max)
print(list(acc))