import numpy as np

a = np.array([[ 0,  1,  2,  3,  4],
       [ 5,  6,  7,  8,  9],
       [10, 11, 12, 13, 14],
       [15, 16, 17, 18, 19],
       [20, 21, 22, 23, 24]])

sub_shape = (3,3)
view_shape = tuple(np.subtract(a.shape, sub_shape) + 1) + sub_shape
print(view_shape)
strides = a.strides + a.strides
print(strides)

sub_matrices = np.lib.stride_tricks.as_strided(a,view_shape,strides)

B = np.array([[1, 1, 1]]*3)
print(B)

D = np.einsum('ijkt,kt->ij', sub_matrices, B)
print(D)
