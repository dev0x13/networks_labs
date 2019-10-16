import time
from multiprocessing import Process
from subprocess import Popen, PIPE
import numpy as np
import re

path_to_agent_bin = "../cmake-build-debug/agent"
path_to_file_to_send = "/home/george/networks_labs/lab1/cfg/test_file"
path_to_tmp_send_cfg = "/tmp/sender.cfg"
path_to_tmp_receive_cfg = "/tmp/receiver.cfg"

regexp = re.compile(r"SendingStats\[packetsToSendNum: (.*?), "
                    r"packetsActuallySentNum: (.*?), "
                    r"efficiencyCoeff: (.*?), "
                    r"totalTimeMs: (.*?)\]")


def write_sender_cfg(protocol, window_size):
    with open(path_to_tmp_send_cfg, "w") as f:
        f.write("mode = SEND\n")
        f.write("mq_id = 1\n")
        f.write("protocol = %s\n" % protocol)
        f.write("window_size = %i\n" % window_size)
        f.write("timeout = 10\n")
        f.write("file = %s\n" % path_to_file_to_send)


def write_receiver_cfg(protocol, loss_probability):
    with open(path_to_tmp_receive_cfg, "w") as f:
        f.write("mode = RECEIVE\n")
        f.write("mq_id = 1\n")
        f.write("protocol = %s\n" % protocol)
        f.write("loss_probability = %f\n" % loss_probability)


def run_agent(cfg):
    proc = Popen([path_to_agent_bin, cfg], stdout=PIPE, stderr=PIPE)
    return proc.communicate()


def run_receiver_agent_in_background():
    p = Process(target=run_agent, args=(path_to_tmp_receive_cfg,))
    p.start()
    time.sleep(0.1)


def run_benchmark(protocol, loss_probability=0.5, window_size=3):
    write_sender_cfg(protocol, window_size)
    write_receiver_cfg(protocol, loss_probability)
    run_receiver_agent_in_background()
    output = [l for l in str(run_agent(path_to_tmp_send_cfg)[0]).split("\\n") if l.startswith("SendingStats")][0]
    output = regexp.search(output)
    return int(output.group(1)), int(output.group(2)), float(output.group(3)), float(output.group(4))


def write_csv(data, file):
    with open(file, "w") as f:
        for row in data:
            for field in row:
                f.write("%s," % str(field))
            f.write("\n")


if __name__ == "__main__":
    # 1. Loss probability bench

    print("Loss probability bench")
    print("----------------------")

    # 1.1. Selective Repeat

    print("Selective Repeat")

    res = []

    for i in np.linspace(0, 0.9, 10):
        r = run_benchmark("SR", loss_probability=i)
        print(i, r)
        res.append([i] + list(r))

    write_csv(res, "loss_prob_SR.csv")

    # 1.1. Go-Back-N

    print("Go-Back-N")

    res = []

    for i in np.linspace(0, 0.9, 10):
        r = run_benchmark("GBN", loss_probability=i)
        print(i, r)
        res.append([i] + list(r))

    write_csv(res, "loss_prob_GBN.csv")

    # 1. Window size bench

    print("Window size bench")
    print("----------------------")

    # 1.1. Selective Repeat

    print("Selective Repeat")

    res = []

    for i in range(1, 11):
        r = run_benchmark("SR", window_size=i)
        print(i, r)
        res.append([i] + list(r))

    write_csv(res, "window_size_SR.csv")

    # 1.1. Go-Back-N

    print("Go-Back-N")

    res = []

    for i in range(1, 11):
        r = run_benchmark("GBN", window_size=i)
        print(i, r)
        res.append([i] + list(r))

    write_csv(res, "window_size_GBN.csv")
