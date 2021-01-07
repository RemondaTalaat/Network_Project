from essential_generators import DocumentGenerator
import sys
import os
import random

node_count = sys.argv[1]
root_dir = "Network_project/simulations/msg_files"
gen = DocumentGenerator()

for idx in range(int(node_count)):
    with open(os.path.join(root_dir, f"{idx}.txt"), "w+") as f:
        message_count = random.randint(10, 20)
        for message in range(message_count):
            f.write(gen.sentence()+"\n")
