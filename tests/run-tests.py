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

    JCUT = [sys.argv[1], "test_group.c", "-t", "test-file.txt", "--"]
    test_report = []
    STDOUT_FILE = "stdout.txt"
    STDERR_FILE = "stderr.txt"
    for group in sorted([dir for dir in os.listdir(os.getcwd()) if "group" in dir]):
        os.chdir(group)
        with open(STDOUT_FILE, 'w') as stdout:
            with open(STDERR_FILE, 'w') as stderr:
                ret = subprocess.call(JCUT, stdout=stdout, stderr=stderr)
        test_report.append((ret, group))
        if os.stat(STDOUT_FILE).st_size == 0:
            os.remove(STDOUT_FILE)
        if os.stat(STDERR_FILE).st_size == 0:
            os.remove(STDERR_FILE)
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
            elif ret < 0 or ret >= 255:
                print("\tJCUT is having problems! debug those issues!")
    else:
        print("SUCCESS!")

    print('\nFinished in {:.3f} seconds'.format(time.clock() - start_time))


if __name__ == "__main__":
    main()
