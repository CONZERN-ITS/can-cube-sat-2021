import sys
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
from scipy.optimize import least_squares

def main():
    if len(sys.argv) < 3:
        print("not enough args")
        return
    elif len(sys.argv) > 3:
        print("too many args")
        return
    infile = open(sys.argv[1], "r")
    outfile = open(sys.argv[2], "w")
    
    data = []
    for line in infile:
        data.append([float(x) for x in line.split()])
        
    mmax = data[0]
    mmin = data[0]
    for x in data:
        for i in range(0, len(x)):
            mmax[i] = max(mmax[i], x[i])
            mmin[i] = min(mmin[i], x[i])
       
    for x in data:
        for i in range(0, len(x)):
            x[i] -= (mmax[i] + mmin[i]) / 2
            
           

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    # For each set of style and range settings, plot n random points in the box
    # defined by x in [23, 32], y in [0, 100], z in [zlow, zhigh].
    c, m, zlow, zhigh = ('r', 'o', -50, -25)
    
    xs = []
    ys = []
    zs = []
    
    for xyz in data:
        xs.append(xyz[0])
        ys.append(xyz[1])
        zs.append(xyz[2])
    ax.scatter(xs, ys, zs, c=c, marker=m)

    ax.set_xlabel('X Label')
    ax.set_ylabel('Y Label')
    ax.set_zlabel('Z Label')

    plt.show()

            
    print(data)
    

 
if __name__ == "__main__":
    main()
