import sys
import os
import random

node_count = sys.argv[1]
root_dir = "Network_project/simulations/msg_files"
messages_corpus = [
    "Hello sweet",
    "how are you?",
    "are you fine?!",
    "great nice seeing ya",
    "wanna coffee?",
    "I hate you",
    "I love you",
    "just kidding",
    "good bye",
    "good for ya",
    "fine!",
    "Hello friend",
    "thanks a lot for your help",
    "thanks in advance!"
]

for idx in range(int(node_count)):
    with open(os.path.join(root_dir, f"{idx}.txt"), "w+") as f:
        message_count = random.randint(20, 50)
        for message in range(message_count):
            if message != message_count - 1:
                f.write(random.choice(messages_corpus)+"\n")
            else:
                f.write(random.choice(messages_corpus))
