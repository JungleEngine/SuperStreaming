from __future__ import print_function, division
import scipy

from keras.datasets import mnist
from keras.layers import Input, Dense, Reshape, Flatten, Dropout, Concatenate
from keras.layers import BatchNormalization, Activation, ZeroPadding2D
from keras.layers.advanced_activations import LeakyReLU
from keras.layers.convolutional import UpSampling2D, Conv2D
from keras.models import Sequential, Model, load_model
from keras.optimizers import Adam
import datetime
import matplotlib.pyplot as plt
import sys
from util.data_loader import DataLoader
import numpy as np
import os


class Discriminator():
    def __init__(self, load_path=None, save_path=None):
        # Input shape
        self.img_rows = 512
        self.img_cols = 512
        self.channels = 3
        self.load_path = load_path
        self.save_path = save_path
        self.img_shape = (self.img_rows, self.img_cols, self.channels)
        # Number of filters in the first layer of  D
        self.df = 64

        optimizer = Adam(0.0002, 0.5)
        # Build and compile the discriminator
        # if self.load_path != None:
        #     self.discriminator = load_model(self.load_path)
        # else:
        self.discriminator = self.build_discriminator()

        self.discriminator.compile(loss='mse',
                optimizer=optimizer,
                metrics=['accuracy'])
        if self.load_path != None:
            self.discriminator.load_weights(load_path)
            print(" Loading existing weights.")


    def build_discriminator(self):

        def d_layer(layer_input, filters, f_size=4, bn=True):
            """Discriminator layer"""
            d = Conv2D(filters, kernel_size=f_size, strides=2, padding='same')(layer_input)
            if bn:
                d = BatchNormalization(momentum=0.8)(d)
            d = LeakyReLU(alpha=0.2)(d)

            return d

        img_A = Input(shape=self.img_shape)
        img_B = Input(shape=self.img_shape)

        # Concatenate image and conditioning image by channels to produce input
        combined_imgs = Concatenate(axis=-1)([img_A, img_B])

        d1 = d_layer(combined_imgs, self.df, bn=False)
        d2 = d_layer(d1, self.df*2)
        d3 = d_layer(d2, self.df*4)
        d4 = d_layer(d3, self.df*8)

        d5 = Conv2D(1, kernel_size=4, strides=1, padding='same')(d4)
        flat = Flatten()(d5)

        output = Dense(1, activation='sigmoid')(flat)
        # output = Dense(2, activation='softmax')(flat)
        return Model([img_A, img_B], output)

    def train(self, dataset_path, epochs=50, batch_size=4, save_interval=1):
        data_loader = DataLoader(path=dataset_path,
                                      img_res=(512, 512))

        start_time = datetime.datetime.now()
        for epoch in range(epochs):
            epoch_loss = 0
            epoch_acc = 0

            for batch_i, (imgs_A, imgs_B, expected_output) in enumerate(data_loader.load_batch(batch_size)):
                (loss, acc) = self.discriminator.train_on_batch([imgs_A, imgs_B], expected_output)
                epoch_loss += loss
                epoch_acc += acc
                elapsed_time = datetime.datetime.now() - start_time
                # Plot the progress
                # print ("[Epoch %d/%d] [Batch %d/%d] [Disc_loss loss: %f, acc: %3d%%] time: %s" % (epoch, epochs,
                #                                                         batch_i, data_loader.n_batches,
                #                                                        loss, 100*acc,
                #                                                         elapsed_time))


            epoch_loss /= data_loader.n_batches
            epoch_acc /= data_loader.n_batches

            val_total_loss = 0
            val_total_acc = 0
            for batch_i,(imgs_A, imgs_B, expected_output) in enumerate(data_loader.load_batch(batch_size, is_testing=True)):
                (loss, acc) = self.discriminator.evaluate([imgs_A, imgs_B], expected_output)
                val_total_loss +=loss
                val_total_acc += acc

            val_total_loss /= data_loader.n_batches
            val_total_acc /= data_loader.n_batches




            print("[Epoch %d/%d] [total_loss: %f, total_acc: %3d%%] "
                  "[validation_loss: %f, validation_acc: %3d%%]" % (epoch, epochs,
                                                                    epoch_loss, 100 * epoch_acc,
                                                                    val_total_loss, 100*val_total_acc
                                                                                             ))
            # TODO save model.
            if epoch % save_interval == 0 and self.save_path != None:
                self.discriminator.save_weights(self.save_path)
                print(" Saving model at epoch %d" %(epoch))


    def predict(self, input):
        result =  np.round(self.discriminator.predict([input[0], input[1]]), 4)
        return result

if __name__ == '__main__':

    discriminator = Discriminator(save_path="/media/syrix/programms/projects/GP/SuperStreaming/benchmark/skipping/models/model2.h5")

    # discriminator.discriminator.summary()
    discriminator.train("/media/syrix/programms/projects/GP/SuperStreaming/benchmark/skipping/dataset")

    # gan = Pix2Pix()
    # gan.train(epochs=200, batch_size=1, sample_interval=200)

    # predict.
    # [1, 0]-> bad
    # [0, 1]->good
    # img_A, img_B = DataLoader.prepare_image(img_path="/media/syrix/programms/projects/GP/test_frames/test_videos/frames/output/"
    #                                "10.31365__2019-03-18 23:43:55.490911.png")
    #
    # print(discriminator.predict((img_A, img_B)))


