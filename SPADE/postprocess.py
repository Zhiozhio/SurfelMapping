
import numpy as np
import os
from PIL import Image
import argparse


if __name__ =='__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--render_dir', type=str)
    parser.add_argument('-s', '--semantic_dir', type=str)
    parser.add_argument('-g', '--gan_gen_dir', type=str)
    parser.add_argument('-t', '--target_dir', type=str, default='./output')
    args = parser.parse_args()

    render_dir = args.render_dir
    semantic_dir = args.semantic_dir
    gan_gen_dir = args.gan_gen_dir
    target_dir = args.target_dir

    if not os.path.exists(target_dir):
        os.makedirs(target_dir)

    for root_A, dirs_A, filenames_A in sorted(os.walk(render_dir)):
        imgs_file = sorted(filenames_A)
    for root_B, dirs_B, filenames_B in sorted(os.walk(gan_gen_dir)):
        img_gen_file = sorted(filenames_B)
    for root_C, dirs_C, filenames_C in sorted(os.walk(semantic_dir)):
        sem_file = sorted(filenames_C)
    

    for name in img_gen_file:
        img_path = os.path.join(render_dir, name)
        img_gen_path = os.path.join(gan_gen_dir, name)
        semantic_path = os.path.join(semantic_dir, name)
        print('***',img_path)
        print('***',img_gen_path)
        print('***',semantic_path)
        img = Image.open(img_path).convert('RGB')
        img_gen = Image.open(img_gen_path).convert('RGB')
        semantic = Image.open(semantic_path)
        img_arr = np.array(img)
        # img_arr.flags.writeable = True
        img_gen_arr = np.asarray(img_gen)
        label = np.asarray(semantic)
        h,w,_ = img_arr.shape #h=378, w=1241
        # print('****',label.shape)
        # print('****', img_arr[100,123])
        # a = np.zeros_like(label)
        # b = np.zeros_like(img_arr)
        for i in range(h):
            for j in range (w):
                if label[i,j]==0:
                    img_arr[i,j]= img_gen_arr[i,j]

        im = Image.fromarray(img_arr)
        im.save(os.path.join(target_dir, name))

        # plt.imshow(im)
        # plt.show()

