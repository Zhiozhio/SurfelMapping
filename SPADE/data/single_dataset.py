"""
Copyright (C) 2019 NVIDIA Corporation.  All rights reserved.
Licensed under the CC BY-NC-SA 4.0 license (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode).
"""

import os.path
from data.base_dataset import BaseDataset, get_params, get_transform
from data.image_folder import make_dataset
from PIL import Image
import util.util as util


class SingleDataset(BaseDataset):

    @staticmethod
    def modify_commandline_options(parser, is_train):
        parser = BaseDataset.modify_commandline_options(parser, is_train)
        # parser.add_argument('--coco_no_portraits', action='store_true')
        parser.set_defaults(preprocess_mode='none')
        if is_train:
            parser.set_defaults(load_size=286)
        else:
            parser.set_defaults(load_size=256)
        # parser.set_defaults(crop_size=256)
        # parser.set_defaults(display_winsize=256)
        parser.set_defaults(crop_size=1248)
        parser.set_defaults(aspect_ratio=3.25)
        parser.set_defaults(display_winsize=512)
        # parser.set_defaults(label_nc=182)
        parser.set_defaults(label_nc=3)
        # parser.set_defaults(contain_dontcare_label=True)
        # parser.set_defaults(cache_filelist_read=True)
        # parser.set_defaults(cache_filelist_write=True)

        parser.add_argument('--label_dir', type=str, required=True,
                            help='path to the directory that contains label images')
        parser.add_argument('--instance_dir', type=str, default='',
                            help='path to the directory that contains instance maps. Leave black if not exists')
        parser.add_argument('--start_frame_id', type=int, default=0,
                            help='test from this id')

        return parser

    def initialize(self, opt):
        self.opt = opt

        label_paths, instance_paths = self.get_paths(opt)

        util.natural_sort(label_paths)
        if not opt.no_instance:
            util.natural_sort(instance_paths)

        label_paths = label_paths[:opt.max_dataset_size]
        instance_paths = instance_paths[:opt.max_dataset_size]

        # if not opt.no_pairing_check:
        #     for path1, path2 in zip(label_paths, image_paths):
        #         assert self.paths_match(path1, path2), \
        #             "The label-image pair (%s, %s) do not look like the right pair because the filenames are quite different. Are you sure about the pairing? Please see data/pix2pix_dataset.py to see what is going on, and use --no_pairing_check to bypass this." % (path1, path2)

        self.label_paths = label_paths
        self.instance_paths = instance_paths

        size = len(self.label_paths)
        self.dataset_size = size

    def get_paths(self, opt):
        label_dir = opt.label_dir
        label_paths = make_dataset(label_dir, recursive=False, read_cache=True)

        if len(opt.instance_dir) > 0:
            instance_dir = opt.instance_dir
            instance_paths = make_dataset(instance_dir, recursive=False, read_cache=True)
        else:
            instance_paths = []

        return label_paths, instance_paths

    def paths_match(self, path1, path2):
        filename1_without_ext = os.path.splitext(os.path.basename(path1))[0]
        filename2_without_ext = os.path.splitext(os.path.basename(path2))[0]
        return filename1_without_ext == filename2_without_ext

    def __getitem__(self, index):
        # Label Image
        label_path = self.label_paths[index]
        label = Image.open(label_path).convert('RGB')
        # apply the same transform to both label and image, where label is a surfel image
        params = get_params(self.opt, label.size)
        transform_label = get_transform(self.opt, params)
        label_tensor = transform_label(label)

        # input image (real images)
        image_tensor = 0

        # if using instance maps
        if self.opt.no_instance or len(self.instance_paths) == 0:
            instance_tensor = 0
        else:
            instance_path = self.instance_paths[index]
            instance = Image.open(instance_path)
            if instance.mode == 'L':
                instance_tensor = transform_label(instance) * 255
                instance_tensor = instance_tensor.long()
            else:
                instance_tensor = transform_label(instance)

        input_dict = {'label': label_tensor,
                      'instance': instance_tensor,
                      'image': image_tensor,
                      'path': label_path,
                      }

        # Give subclasses a chance to modify the final output
        self.postprocess(input_dict)

        return input_dict

    def postprocess(self, input_dict):
        return input_dict

    def __len__(self):
        return self.dataset_size


