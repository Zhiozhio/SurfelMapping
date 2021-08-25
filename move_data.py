# move generated data, especially novel data, to destinate dir without conflict

import os
import argparse

novel_path = './output/novel'
data_dir_list = ['image', 'semantic']

parser = argparse.ArgumentParser()
parser.add_argument('--offset', type=int, help='frame number offset (add)')
parser.add_argument('-t', '--destination', type=str, help='destination directory')
parser.add_argument('-f', '--fake', action='store_true', help='if set, only print operations but not really implement')

args = parser.parse_args()
offset = args.offset
dest_dir = args.destination

for data_dir in data_dir_list:
    full_data_dir = os.path.join(novel_path, data_dir)
    data_list = os.listdir(full_data_dir)
    full_dest_data_dir = os.path.join(dest_dir, data_dir)

    # print(data_list)

    for data in data_list:
        base_id = int(data.split('.')[0])
        dest_data = '{:0>6d}.png'.format(base_id + offset)

        print('mv {} {}'.format(os.path.join(full_data_dir, data), os.path.join(full_dest_data_dir, dest_data)))

        if not args.fake:
            os.system('mv {} {}'.format(os.path.join(full_data_dir, data), os.path.join(full_dest_data_dir, dest_data)))
