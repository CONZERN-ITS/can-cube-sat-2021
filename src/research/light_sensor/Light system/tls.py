import numpy.linalg as lg
import numpy as np
import random
import math
from abc import ABCMeta, abstractmethod
from numpy import pi, cos, sin, arccos, arange

import mpl_toolkits.mplot3d

import matplotlib.pyplot as pp



#important!!!!
#all functions return lists. Even if it's named get_..._matrix

#values for rejecting sensors that not under sun light
min_flow_abs = 0.05
min_flow_rel = 0.05

#simulate_random parametres
random_rel = 0.01
random_abs = 0.01
sensor_dev_rel = 0
sensor_dev_abs = 0

count_of_sensors = 8

#bigger - more accurate test_system
count_of_rounds = 10000

#maximum of light power
lmin = 1
lmax = 5
#help functions
def euler_to_dekart(psi, fi):
    psi = math.radians(psi)
    fi = math.radians(fi)
    x = (math.cos(fi) * math.cos(psi))
    y = (math.cos(fi) * math.sin(psi))
    z = (math.sin(fi))

    return [x,y,z]


def norm(a):
    s = 0
    for i in a:
        s += i**2
    return s**0.5



def find_angle(a1,a2):
    a1 = normate(a1)
    a2 = normate(a2)
    s = 0
    for i in range(len(a1)):
        s += a1[i] * a2[i]
    return math.acos(s)


def dekart_to_euler(a):
    x = a[0]
    y = a[1]
    z = a[2]
    fi = find_angle(a, [0,0,1])
    psi = find_angle([x, y, 0], [1, 0, 0])

    if x > 0 and y < 0:
        psi = -psi
    elif x < 0 and y < 0:
        psi = -psi
    return [math.degrees(psi), 90 - math.degrees(fi)]

def normate(a):
    s = norm(a)
    if(s == 0):
        return a

    res = []
    for i in a:
        res.append(i/s)
    return res

def square_deviation(a1, a2):
    s = []
    for i in range(len(a1)):
        s.append(a1[i] - a2[i])
    return norm(s)/len(s)**0.5

#returns deviation in degrees
def test_system(min_height,max_height):

    ls = Light_system()

    deviation = 0

    i = 0
    minZ = 0
    while(i < count_of_rounds):
        x = random.uniform(-1, 1)
        z = random.uniform(min_height, max_height)
        pp = x**2 + z**2
        if pp > 1:
            continue
        y = (1 - pp)**0.5
        m = random.uniform(min, max)
        x *= m
        y *= m
        z *= m

        b = ls.simulate_random([x, y, z])

        res = ls.get_solution(b)
        if(isinstance(res, str)):
            continue

        dev = math.cos(find_angle([x,y,z],res))

        dev = dev**2
        deviation += dev
        i += 1

    deviation = ((deviation / count_of_rounds)**0.5)
    return math.degrees(math.acos(deviation))
    #return deviation


#will be deleted in next versions
def get_solve_matrix(matrix):
    a = np.matrix(matrix)
    """
    print(a)
    print(a.getI())
    b = np.matmul(a,a.getI())

    x = np.matrix([1,1,1]).getT()
    res = np.matmul(a, x)
    print(b.getI())
    print(b)
    """
    return a.getI()

#is not used in this version
def restrainMatrix(matrix, choise):
    res = []
    mask = 1
    for l in matrix.tolist():
        if(mask & choise):
            res.append(r)
        k = k << 1
    return res


def save_solve_matrix(f, matrix):
    for l in matrix.getT().tolist():
        for a in l:
            f.write("{0:f} ".format(a))
        f.write('\n')

def set_properties():
    m = get_sensor_matrix()
    f = open("light_sensors_properties","w")
    #f.write("{0:b} ".format(choise))
    save_solve_matrix(f, get_solve_matrix(m))
    f.close();


class Light_system_base(metaclass=ABCMeta):
    sensors_vectors = []
    def __init__(self, sensor_vectors):
        self.sensor_vectors = sensor_vectors
    @abstractmethod
    def get_light_vector(self, sensor_values):
        pass
    
class Light_system_linear(Light_system_base):
    __sensor_matrix = []
    __all_solve_matrixes = []
    def __get_all_solve_matrixes(self, sensor_matrix):
        l = sensor_matrix
        r = []
        r.append(None)
        for i in range(2**len(sensor_vectors)):
            if(i == 0):
                continue
            s = []
            j = i
            for k in l:
                if(j % 2 == 1):
                    s.append(k)
                j = j >> 1
            k = np.matrix(s)

            j = i
            if(lg.matrix_rank(k) == 3):
                h = k.getI().getT().tolist()
                d = []
                ii = 0
                for k in l:
                    if(j % 2 == 1):
                        d.append(h[ii])
                        ii += 1
                    else:
                        d.append([0.0,0.0,0.0])
                    j = j >> 1
                h = np.matrix(d).getT()
                #print(h.getT())
                r.append(h)
            else:
                r.append(None)
        return r

    def __find_solution(self, solve_matrix, sensor_values):
        sensor_values.reverse()
        res = np.matmul(np.matrix(solve_matrix), np.matrix(sensor_values).T).tolist()
        res = [a[0] for a in res]
        return res

    def __get_good_solve_matrix(self, sensor_values):
        l = sensor_values
        a = 0
        l.reverse()
        s = 0
        for i in l:
            s += i
        s /= len(l)
        for i in l:
            a = a << 1
            if(i > s * min_flow_rel + min_flow_abs):
                a += 1

        return self.__all_solve_matrixes[a]


    def __init__(self):
        self.__all_solve_matrixes = self.__get_all_solve_matrixes(self.sensor_vectors)

    def get_light_vector(self, sensor_values):
        a = self.__get_good_solve_matrix(sensor_values)
        if a is None:
            return None

        return self.__find_solution(a, sensor_values)

class Light_system_gradient(Light_system_base):
    def __init__(self, sensor_vectors, sensor_function, grad, koef1, koef2, beta, n):
        super().__init__(sensor_vectors)
        self.sensor_function = sensor_function
        self.grad = grad
        self.koef1 = koef1
        self.koef2 = koef2
        self.beta = beta
        self.n = n
    xx = []
    zz = []
    dV = [0,0,0,0]
    alpha = 0
    beta = 0
    auto_start_vector = True 
    start_vector = [1,0,0,0]
    def __one_step(self, sensor_values, vector_x):
        
        J = np.array([]).reshape(4,0)
        f = np.array([]).reshape(0,1)

        x = np.array(vector_x[:3]).reshape(3,1)
        d = vector_x[3]
        for ai in self.sensor_vectors:
            a = np.array(ai).reshape(3,1)
            a_x = np.dot(a.ravel(), x.ravel())
            n = np.linalg.norm(x)
            n2= n * n
            fi = self.sensor_function(a_x / n) * n + d
            gi = self.grad(a_x / n)
            Ji = gi * a + (fi - gi * a_x) / n2 * x

            Ji = np.vstack((Ji, 1))


            J = np.hstack((J, Ji))
            f = np.vstack((f, fi))
        dif = f - np.array(sensor_values).reshape(f.shape)
        self.zz.append(np.dot(dif.ravel(),dif.ravel()))
        grad = np.matmul(J, dif)

        grad = grad / np.linalg.norm(grad) 
        x1 = (np.array(vector_x).reshape(4, 1) - self.alpha * grad + self.beta * np.array(self.dV).reshape(4,1))
        self.dV = x1 - np.array(vector_x).reshape(4, 1)
        return x1.ravel()
    def __multi_step(self, sensor_values, n):
        x = self.start_vector
        x1 = self.start_vector
        dV = [0,0,0,0]
        if self.auto_start_vector:
            index = 0
            for i in range(1, len(sensor_values)):
                if sensor_values[i] > sensor_values[index]:
                    index = i
            x1 = self.sensor_vectors[index].copy()
            x1[0] += 0.05

            x1.append(0)
                
        for i in range(0, n):
            self.alpha = math.e**(-i*self.koef1 + self.koef2)
            x = x1
            x1 = self.__one_step(sensor_values, x)
            self.xx.append(x)
        return (x, x1)

    def get_light_vector(self, sensor_values):
        return self.__multi_step(sensor_values, self.n)[1]

def load_sensors(filename):
    f = open(filename,"r")
    b = (f.read().split('\n'))
    d = []
    for i in b:
        d.append(i.split(' '))
    f.close();
    d.pop()
    s = []
    for l in d:
        r = euler_to_dekart(float(l[0]), float(l[1]))
        s.append(r)
    return s

   

class LS_simulation_base:
    def __init__(self, lsb):
        self.lsb = lsb
    def start_simulation(self):
        pass

class LS_simulation_fib:
    def __init__(self, lsb, num_pts, rand_f):
        self.lsb = lsb
        self.num_pts = num_pts
        self.rand_f = rand_f

    def start_simulation(self): 
        num_pts = self.num_pts
        indices = arange(0, num_pts, dtype=float) + 0.5
        phi = arccos(1 - 2*indices/num_pts)
        theta = pi * (1 + 5**0.5) * indices
        x, y, z = cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi);
        print(x.shape)
        print(x[0])
        f = []     
        g = []

        sen_value = lambda x, a : self.rand_f(lsb.sensor_function((x[0]*a[0] + x[1]*a[1] + x[2]*a[2])/norm(x)/norm(a)))

        xx = [0,1,0]
        for i in range(0, x.size):
            sen = []
            xx = [x[i], y[i], z[i]] 
            ff = lambda : random.uniform(-1, 1)
            lsb.start_vector = [ff(), ff(), ff(), 0]


            lsb.zz = []
            lsb.xx = []
            for a in lsb.sensor_vectors:
                sen.append(sen_value(xx, a))
            r = lsb.get_light_vector(sen)
            print(lsb.zz)
            d = r[3]
            r = r[:3]
            print(d)
            n = norm([x[i], y[i], z[i]])
            dif = math.acos(min(max(np.dot(r,xx)/norm(xx)/norm(r), 0),1)) / 2 / math.pi * 360
            print(dif)
            f.append(dif) 
        sen = []
        xx = [0,1,0]
        xx = normate(xx)
        #f = lambda : random.uniform(-1, 1)
        #xx = [f(), f(), f()]
        #xx = normate(xx)
        for a in lsb.sensor_vectors:
            sen.append(lsb.sensor_function(xx[0]*a[0] + xx[1]*a[1] + xx[2]*a[2]))
            if  xx[0]*a[0] + xx[1]*a[1] + xx[2]*a[2]>= 1:
                print("Oh no: ", xx, a) 
        for i in range(0, x.size):
            f1 = np.array([]).reshape(0,1)
            x0 = np.array([x[i], y[i], z[i]]).reshape(3,1)
            for ai in lsb.sensor_vectors:
                a0 = np.array(ai).reshape(3,1)
                a_x = np.dot(a0.ravel(), x0.ravel())
                n0 = np.linalg.norm(x0)
                fi = lsb.sensor_function(a_x / n0) * n0
                if a_x/n0 >= 1:
                    print("Oh no1: ", a0, x0)
                f1 = np.vstack((f1, fi))
            dif2 = (f1.ravel() - sen)
            dif2 = np.dot(dif2, dif2)
            g.append(dif2)

        print("variation: ", sum(f)/len(f))
        fig = pp.figure()
        plot = fig.add_subplot(111, projection='3d')
        #plot.scatter((theta % (2*math.pi)) / 2 / math.pi * 360, (phi % (2*math.pi))/ 2 / math.pi * 360, g);
        #plot.scatter([dekart_to_euler(v[:3])[0]%360 for v in lsb.xx], [90 - dekart_to_euler(v[:3])[1]for v in lsb.xx], [z for z in lsb.zz], color = 'red', marker = '*');
        plot.scatter(theta % (2*math.pi) / 2 / math.pi * 360, phi%(2*math.pi)/ 2 / math.pi * 360, f);
        
        pp.xticks(range(0, 360, 30))
        pp.yticks(range(0, 180, 30))
        pp.show()


class Light_system:
    count_of_sensors = 0
    __sensor_matrix = []
    __all_solve_matrixes = []
    def __get_sensor_matrix(self):
        f = open("sensor_values","r")
        b = (f.read().split('\n'))
        d = []
        for i in b:
            d.append(i.split(' '))
        f.close();
        d.pop()
        s = []
        for l in d:
            r = euler_to_dekart(float(l[0]), float(l[1]))
            s.append(r)
        return s

    def __get_all_solve_matrixes(self, sensor_matrix):
        l = sensor_matrix
        r = []
        r.append(None)
        for i in range(2**count_of_sensors):
            if(i == 0):
                continue
            s = []
            j = i
            for k in l:
                if(j % 2 == 1):
                    s.append(k)
                j = j >> 1
            k = np.matrix(s)

            j = i
            if(lg.matrix_rank(k) == 3):
                h = k.getI().getT().tolist()
                d = []
                ii = 0
                for k in l:
                    if(j % 2 == 1):
                        d.append(h[ii])
                        ii += 1
                    else:
                        d.append([0.0,0.0,0.0])
                    j = j >> 1
                h = np.matrix(d).getT()
                #print(h.getT())
                r.append(h)
            else:
                r.append(None)
        return r

    def __find_solution(self, solve_matrix, sensor_values):
        sensor_values.reverse()
        res = np.matmul(np.matrix(solve_matrix), np.matrix(sensor_values).T).tolist()
        res = [a[0] for a in res]
        return res

    def __get_good_solve_matrix(self, sensor_values):
        l = sensor_values
        a = 0
        l.reverse()
        s = 0
        for i in l:
            s += i
        s /= len(l)
        for i in l:
            a = a << 1
            if(i > s * min_flow_rel + min_flow_abs):
                a += 1

        return self.__all_solve_matrixes[a]


    def __init__(self):
        self.__sensor_matrix = self.__get_sensor_matrix()
        self.count_of_sensors = len(self.__sensor_matrix)
        self.__all_solve_matrixes = self.__get_all_solve_matrixes(self.__sensor_matrix)

    def get_solution(self, sensor_values):
        a = self.__get_good_solve_matrix(sensor_values)
        if a is None:
            return None

        return self.__find_solution(a, sensor_values)

    #simulates sensor values
    def simulate_random(self, light):
        r = []
        #best way to find norm of light
        M = norm(light)
        #return simulate(sensor_matrix, light, random.random() * random_rel, random.random() * random_abs + sensor_dev_rel * M)

        for i in self.__sensor_matrix:
            s = 0
            for j in [0,1,2]:
                s += i[j] * light[j]
            if s < 0:
                s = 0

            r1 = random.random() * random_rel
            r2 = random.random() * random_abs

            s = (1 + r1) * s + r2 + sensor_dev_rel * M + sensor_dev_abs
            r.append(s)

        return r


    def simulate(self, light):

        r = []
        for b in self.__sensor_matrix:
            s = 0

            for i in range(len(light)):
                s += light[i] * b[i]

            if(s < 0):
                s = 0
            r.append(s)
        return r

    def get_sensor_matrix(self):
        return self.__sensor_matrix


A = 1/2
def f(x):
    if x > 1:
        print("wow", x)
    if x > 0 and x <= 1:
        return x**A
    else:
        return 0

def g(x):
    if x > 0 and x <= 1:
        return A* x**(A - 1)
    else:
        return 0

def f3(x):
    if x > 1:
        print("wow", x)
    if x > 0 and x <= 1:
        return 4.19 * (x+1.44)**A - 4.63
    else:
        return 0

def g3(x):
    if x > 0 and x <= 1:
        return 4.19 * A* (x+1.44)**(A - 1)
    else:
        return 0

def r_f(x):
    return x + random.gauss(0.5, 0.1) 





if __name__ == "__main__":
    sensors = load_sensors("sensor_values")
    #f1 = lambda x: math.e**(-5.35*math.acos(x)*math.acos(x) + 0.000276)
    #f2 = lambda x: f1(x) * -2 * 5.35 * math.acos(x) / -(1 - x*x)**0.5 * np.sign(x) 
    #lsb = Light_system_gradient(sensors, f1, f2, 0.2, 300) 
    #lsb = Light_system_gradient(sensors, lambda x : x if x > 0 else 0, lambda x : 1 if x > 0 else -0.1, 0.15, 0.5, 0.5, 25) 
    lsb = Light_system_gradient(sensors, f, g, 0.10, 0.1, 0.1, 10) 
    #lsb = Light_system_gradient(sensors, lambda x: 0.90*x+0.13 if x > 0 else 0, lambda x: 0.9 if x > 0 else 0, 0.25, 0.8, 0.5, 25)
    lss = LS_simulation_fib(lsb, 1000, lambda x : 1*r_f(x))
    lss.start_simulation()

