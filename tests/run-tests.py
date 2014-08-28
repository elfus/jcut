#!/usr/bin/python3
import os
import sys
import subprocess
import time


def main():
    cur_folder = os.getcwd().split("/")[-1]
    if not cur_folder == "tests":
        print("The script ", sys.argv[0], " hast to be executed under the 'tests' folder")
        exit(1)

    if len(sys.argv) < 2:
        print("You need to supply the absolute path to the jit-testing binary.")
        print("\nUsage:")
        print("\t", sys.argv[0], " /path/to/jit-testing")
        exit(1)

    start_time = time.clock()

    JCUT = [sys.argv[1], "test_group.c", "-t", "test-file.txt", "--recover"]
    test_report = []
    for group in sorted([dir for dir in os.listdir(os.getcwd()) if "group" in dir]):
        os.chdir(group)
        with open("stdout.txt", 'w') as stdout:
            with open("stderr.txt", 'w') as stderr:
                ret = subprocess.call(JCUT, stdout=stdout, stderr=stderr)
        test_report.append((ret, group))
        os.chdir("..")

    # Print test summary
    count = sum([x for x, y in test_report if x < 255])
    if count:
        print("FAILED")
        failed_dir = [dir for ret, dir in test_report if ret == 255]
        if len(failed_dir) > 0:
            print("Failed to run directories: ", failed_dir)
        print("Total number of tests failed: ", count)
        for ret, group in test_report:
            if 0 < ret < 255:
                print("\tDirectory", group, "has", ret, "failed test(s)")
    else:
        print("SUCCESS!")

    print('\nFinished in {:.3f} seconds'.format(time.clock() - start_time))


if __name__ == "__main__":
    main()
