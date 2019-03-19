import scipy
from glob import glob
import numpy as np
import os
import cv2
import matplotlib.pyplot as plt

class DataLoader():
    def __init__(self, path, img_res=(512, 512)):
        self.path = path
        self.img_res = img_res
        self.check_something = 10
        self.check_something2 = 10

    def load_data(self, batch_size=1, is_testing=False):
        data_type = "train" if not is_testing else "test"
        path = glob('./datasets/%s/%s/*' % (self.dataset_name, data_type))

        batch_images = np.random.choice(path, size=batch_size)

        imgs_A = []
        imgs_B = []
        for img_path in batch_images:
            img = self.imread(img_path)

            h, w, _ = img.shape
            _w = int(w/2)
            img_A, img_B = img[:, :_w, :], img[:, _w:, :]

            img_A = scipy.misc.imresize(img_A, self.img_res)
            img_B = scipy.misc.imresize(img_B, self.img_res)

            # If training => do random flip
            if not is_testing and np.random.random() < 0.5:
                img_A = np.fliplr(img_A)
                img_B = np.fliplr(img_B)

            imgs_A.append(img_A)
            imgs_B.append(img_B)

        imgs_A = np.array(imgs_A)/127.5 - 1.
        imgs_B = np.array(imgs_B)/127.5 - 1.

        return imgs_A, imgs_B

    def load_batch(self, batch_size=1, is_testing=False):
        data_type = "train" if not is_testing else "val"
        path = glob(self.path+'/%s/*' % (data_type))
        labels_sigmoid = [os.path.basename(x)[0] for x in path]
        labels_softmax = []
        for l in labels_sigmoid:
            if int(l) == 1:

                # To be removed.
                self.check_something -= 1
                if self.check_something > 0:
                    print("label == 0 found")

                # labels_softmax.append([1, 0])
                labels_softmax.append([1])
            else:

                # To be removed.
                self.check_something2 -= 1
                if self.check_something2 > 0:
                    print("label == 1 found")

                labels_softmax.append([0])
        self.n_batches = int(len(path) / batch_size)

        for i in range(self.n_batches-1):
            batch = path[i*batch_size:(i+1)*batch_size]
            batch_labels = labels_softmax[i*batch_size:(i+1)*batch_size]
            l = []
            for s in batch_labels:
                l.append(s)
            batch_labels = np.asanyarray(batch_labels)
            imgs_A, imgs_B= [], []
            for img in batch:
                img = self.imread(img)
                h, w, _ = img.shape
                half_w = int(w/2)
                img_A = img[:, :half_w, :]
                img_B = img[:, half_w:, :]

                img_A = scipy.misc.imresize(img_A, self.img_res)
                img_B = scipy.misc.imresize(img_B, self.img_res)
                # img = np.uint8(img_A)
                # img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
                # cv2.imshow("img", img)
                # cv2.waitKey(0)

                # if not is_testing and np.random.random() > 0.5:
                #         img_A = np.fliplr(img_A)
                #         img_B = np.fliplr(img_B)

                imgs_A.append(img_A)
                imgs_B.append(img_B)

            imgs_A = np.array(imgs_A)/127.5 - 1.
            imgs_B = np.array(imgs_B)/127.5 - 1.

            yield imgs_A, imgs_B, batch_labels

    @staticmethod
    def prepare_image(img_path, res=(512, 512)):
        img = scipy.misc.imread(img_path, mode='RGB').astype(np.float)
        h, w, _ = img.shape
        half_w = int(w / 2)
        img_A = img[:, :half_w, :]
        img_B = img[:, half_w:, :]
        img_A = [scipy.misc.imresize(img_A, res)]
        img_B = [scipy.misc.imresize(img_B, res)]

        img_A = np.array(img_A) / 127.5 - 1.
        img_B = np.array(img_B) / 127.5 - 1.
        return img_A, img_B

    def imread(self, path):
        return scipy.misc.imread(path, mode='RGB').astype(np.float)