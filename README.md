# Date: Dec 2019 

My implementation of a shared concurrent (multithreaded using pthread library) red black tree using readers/writers algorithm (priority with readers)

# To run the program:

1- The input file can only have one test case at a time. please see the sample input file that I attached to see the format (which is identical to the sample tests provided). 

2- Please make sure that the input file's name is put into the code. My input file is called "input.txt" so if your testing with a different name, please go to line 160 and change that to yours.

3- Simply type in "make" then "./a.out" which will print the results to the console & produce an output file with the results as well as specified in the project description. The name of the output file will be "output.txt". A sample output file is attached. 

4- That output file has the output of each search operation followed by the thread ID executing the operation, execution time, and the final red-black tree(in preorder), in that order. 

To see the concurrency: please run the program multiple times to see different orders and results (order depends how the scheduler schedules them which we have no control over). 

Thank you!
Fadel 
