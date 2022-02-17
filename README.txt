Once connected to the network, each user will be given a unique key of length 5 composed of capital letters. This is done using the gen_random function at the beginning of main. 

Whenever a user sends a message, his or her key will be written into a secret.txt file. By default, every user connected to the network will receive the message encrypted using the key of the sender.

Each user can designate a receiver so that only the designated receiver can read the message the sender is trying to convey. However, the receiver will have to type ‘:true’ to receive the original message setting the isReceiver boolean value to be true. Typing ‘:false’, isReceiver goes back to false state and will receive the encrypted message just like everyone else. 

The implementation includes cipherText which takes in the ciphered text, original text, and key. Every sequence of 5 letters in the original text will be shifted by the value of the key and turn it into the corresponding capital letter. originalText does the exact opposite to decrypt the already encrypted message.

Attached alongside is the proof that our code successfully decrypted the encrypted message. Somehow, the code started decrypting the message in a weird manner due to an unknown factor. 
