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
    "thanks in advance!",
    "this is a new year",
    "let's do something new",
    "let us try that",
    "good dog",
    "good boy you are",
    "random message generation",
    "collapse!!!",
    "good year",
    "awesome experience",
    "new message",
    "good old days",
    "everyone must face the consequences of their own deeds",
    "maximum speed violation",
    "european people are restrictive",
    "what about the new day",
    "finished my work early"
]

for idx in range(int(node_count)):
    messages_copy = list(messages_corpus)
    with open(os.path.join(root_dir, f"node{idx+1}.txt"), "w+") as f:
        message_count = random.randint(15, 30)
        for message in range(message_count):
            message_txt = random.choice(messages_copy)
            messages_copy.remove(message_txt)
            if message != message_count - 1:
                f.write(message_txt+"\n")
            else:
                f.write(message_txt)
