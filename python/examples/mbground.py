from MeshKit import *

file_name = "../data/PB.h5m"
outfile = "out.h5m"
print file_name 

mk = MKCore()

mk.load_mesh(file_name)

surfs = MEntVector()
mk.get_entities_by_dimension(2, surfs)

qm = mk.construct_meshop("MBSplitOp", surfs)

direction = [0.,  0.,  1. ]
poly = [537200,  7680200,  -2000., 537800,  7680100,  -2000., 537700,  7680980,  -2000.,  537400 , 7680930 , -2000. ,537100 ,7680900 , -2000.]
gid = 1
closed = 0
qm.set_options(gid, direction[0], direction[1], direction[2], closed)
qm.add_points(537000  , 7680445,  150.8383)
qm.add_points(537500 ,  7680300  , -2000.)
qm.add_points(537700 , 7680400 , -2000.)
qm.add_points(538000 , 7680446 ,196.5589)

mk.setup_and_execute()

mk.save_mesh(outfile)
