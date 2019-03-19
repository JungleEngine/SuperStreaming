import os
import cv2
import numpy as np
from glob import glob
import subprocess
import optparse

SPACE = 32
LEFT_ARROW = 81
RIGHT_ARROW = 83

good_image_char = '0'
bad_image_char = '1'

processed_imgs = []  # [img, name]
current_processing_index = 0



if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-i', '--input_path',
                      action="store", dest="input_path",
                      help="input_path", default="/media/syrix/programms/projects/GP/SuperStreaming/benchmark/skipping/images")

    parser.add_option('-o', '--output_path',
                      action="store", dest="output_path",
                      help="output_path", default="/media/syrix/programms/projects/GP/SuperStreaming/benchmark/skipping/images")

    options, args = parser.parse_args()




    input_path = options.input_path
    output_path = options.output_path
    imgs_paths = glob('%s/*' % (input_path))
    tag = {}
    for x in imgs_paths:
        tag[os.path.basename(x)] = -1

    print(
        " press (SPACE) to label image as good\n"
        " press (->) to proceed to next image\n"
        " press (<-) to go back\n"
        " press (S) to save current processed images\n"
        " press (Q) to close app\n")

    close_app = False
    while not close_app:
        img = cv2.imread(imgs_paths[current_processing_index])
        img_name = os.path.basename(imgs_paths[current_processing_index])
        img_folder_path = os.path.dirname(imgs_paths[current_processing_index])

        title = " Please label our image Mr. labeler"
        cv2.imshow(title, img)



        while True:
            key_pressed = cv2.waitKey(0)
            if key_pressed == SPACE:  # Label image as good.
                new_image_path = output_path + '/' + good_image_char + img_name
                tag[img_name] = 1
                print(" Image labeled as good.")

                break

            # if key_pressed == ord('2'):  # Label image as bad.
            #     new_image_path = output_path + '/' + bad_image_char + img_name
            #     tag[img_name] = 2
            #     break

            if key_pressed == RIGHT_ARROW:  # Proceed.

                if tag[img_name] == -1:
                    tag[img_name] = 2
                    print(" Image labeled as bad.")

                current_processing_index += 1
                current_processing_index = min(current_processing_index, imgs_paths.__len__() - 1)
                break


            if key_pressed == LEFT_ARROW:  # Go back.
                current_processing_index -= 1
                current_processing_index = max(current_processing_index, 0)
                break

            if key_pressed == ord('s'):  # Save.
                # for img, name in processed_imgs:
                    # cv2.imwrite(name, img)
                n_saved = 0
                for key, value in tag.items():
                    if value == 1:  # Good
                        subprocess.run(["cp", input_path+"/"+key, output_path+'/'+good_image_char+key])
                        n_saved += 1
                        tag[key] = -2
                    if value == 2:  # Bad
                        subprocess.run(["cp", input_path+"/"+key, output_path+'/'+bad_image_char+key])
                        n_saved += 1
                        tag[key] = -2


                print(" (%d) Image saved" %(n_saved))
                # processed_imgs.clear() #clear array
                break

            if key_pressed == ord('q') or cv2.getWindowProperty(title, 1) == -1: # Close all
                cv2.destroyAllWindows()
                close_app = True
                break



