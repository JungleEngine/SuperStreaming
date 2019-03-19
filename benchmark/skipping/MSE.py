import cv2
import matplotlib.pyplot as plt
import numpy as np


def append_and_compute_diff_two_images(img1, img2, text, margin=50):
    if img1.shape[2] != img2.shape[2]:
        print(" Different pixel format")
        return None

    diff = np.sum(np.abs(img1 - img2)) / (img1.shape[0] * img1.shape[1] * img1.shape[2])
    x = np.zeros([img1.shape[0] + margin,
                  img1.shape[1] * 2 + margin, img1.shape[2]], dtype=np.uint8)
    x.fill(255)
    x[0:img1.shape[0], 0:img1.shape[1]] = img1

    x[0:img2.shape[0],
    img1.shape[1] + margin:img1.shape[1] + img2.shape[1] + margin] = img2

    return x, diff


if __name__ == '__main__':
    base_path = "/media/syrix/programms/projects/GP/test_frames/test_videos/frames/"
    start_indx = 1
    end = 495
    i = 265
    img1 = cv2.imread(base_path + str(i - 1) + ".png")
    ground_truth = cv2.imread(base_path + str(i) + ".png")
    img3 = cv2.imread(base_path + str(i + 1) + ".png")
    interpolated_img = (img1 / 2 + img3 / 2)
    # result, diff = append_and_compute_diff_two_images(interpolated_img, ground_truth, "")
    cv2.imwrite(base_path + "output_single_frames/" + "__" + str(i) + ".png", interpolated_img)

    # for i in range(2, 496):
    #     img1 = cv2.imread(base_path + str(i - 1) + ".png")
    #     ground_truth = cv2.imread(base_path + str(i) + ".png")
    #     img3 = cv2.imread(base_path + str(i + 1) + ".png")
    #     interpolated_img = (img1 / 2 + img3 / 2)
    #     result, diff = append_and_compute_diff_two_images(interpolated_img, ground_truth, "")
    #     cv2.imwrite( base_path + "output/" + "{0:.5f}".format(diff) + "__" + str(i) + ".png", result)
    #     print(diff)
        # cv2.destroyAllWindows()
        # cv2.imshow("interp_gtruth_" + str(i) + ".png", result)
        # cv2.waitKey(0)


        # img = cv2.imread(base_path+"42.png")

        # img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)


        # plt.savefig("/media/syrix/programms/projects/GP/test_frames/test_videos/frames/test")
