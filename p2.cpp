/*
Fadel Alshammasi
Project 2 COM S 352
12/8/19
Implementation of a shared concurrent (multithreaded using pthread library) red black tree using readers/writers algorithm (priority with readers)
*/
#include <chrono>
#include<algorithm>
#include<iostream> 
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include<stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <queue> 
#include <ctime>

using namespace std;

auto start = chrono::steady_clock::now(); //start the timer


int readcount=0;
pthread_mutex_t x; //mutex to ensure readcout is updated proprely 
pthread_mutex_t wsem; //mutex to ensure mutual exclusion 

// a lock to ensure that the output of the search operations in the output file is not corrupted or interleaving because although multiple searches can search at the same time all together, only one search can write the result to the oputput file at a time (so I only synchronize printing the result of the search to a file, not the actual search)
pthread_mutex_t writeSafelyToFile; 
 

int numOfSearchT; 
int numOfModifyT;

queue <int>searchQ; //the search queue 
queue <string>modifyQ; //the modify queue


bool keepTrackSearch=false;
int largestKey;

ofstream file("output.txt");


enum Color {RED, BLACK}; 

/* A struct to create a Node */
typedef struct Node
{
   int key;
   struct Node* left;
   struct Node* right;
   struct Node *parent;
   bool color;
       
   Node(int key){
       this ->key=key;
       left=right=parent=nullptr;
       this ->color=RED;
   }

   Node (int key, bool color){
       this ->key=key;
       left=right=parent=nullptr;
       this ->color=color;
   }
} *node_t;

/* A red black tree class with a root and a sentinel nill node to represent the leaves*/
class RBT{
    public:
    node_t root;
    node_t nillNode;
        RBT(){
           initialize(); //construct the initial tree from the input file in preorder 
        } 
        /* Red black tree member functions' prototypes*/

         void insertHelper(int e); 
         void leftRotate (node_t x);
         void rightRotate(node_t x);
         void RBInsertFixup (node_t z);
         void deleteHelper (node_t x, int e);
         void RBTransplant (node_t u, node_t v);
         void RBDeleteFixup (node_t x);
         node_t iniTreeHelper (vector <string> elements, int *preIndex, int low, int high, int size);
         node_t iniTree (vector<string>elements,int size);
         node_t minimum (node_t node);
         void printPreOrder (node_t node) ;
         void initialize ();
         void linkNill(node_t node);
         node_t linkParent(node_t node);
         int getPostionOfLastF(vector <string> v);
         string deleteSpaces(string &str);
         int maxKey(node_t node);
         void  ReplaceStringInPlace(std::string& subject, const std::string& search,const std::string& replace);
              
};

RBT rbt;
/*
 A function to help extracting functions arguments from the input file
@pararm str: string to extract token from, beg: char to begin the extraction at. end: char to end the extraction at 
@return the argument (which is a number) in a string format 
*/ 
std::string extract(const std::string& str, char beg, char end)
{
    std::size_t begPos ;
    if ( (begPos = str.find(beg)) != std::string::npos )
    {
        std::size_t endPos ;
        if ( (endPos = str.find(end, begPos)) != std::string::npos && endPos != begPos+1 )
            return str.substr(begPos+1, endPos-begPos-1) ;
    }

    return std::string() ;
}

/*
A helper function to help manpulating input to help in the input file
@return void
*/
void RBT::ReplaceStringInPlace(std::string& subject, const std::string& search,const std::string& replace){
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }

}

/*

A helper function to help in parsing the input file by getting the postion of the last nil node
@pararm a string vector that has the intial tree in preorder
@return postion of last f (nilnode)
*/
int RBT::getPostionOfLastF(vector <string> v){
    int i;
    for(i=v.size()-1; i>=0 ; i--){
        string current=v[i];
        if (current.at(0)=='f'){
            break;
        }
    }
    return i;

}

/*
A function to help initalizing the intial read black tree and doing all the parsing stuff from the input file like getting 
the number of search and modify threads and filling the queues with the function calls from the input file
*/
void RBT::initialize(){
    
 ifstream infile;
    infile.open("input.txt"); 

        std:: string line;
         vector<string> words;
        
        /*store inital content of the tree in a vector */
        while(getline(infile,line)){
            stringstream ss(line);
            while(getline(ss,line,',')){
                words.push_back(line);
            }
        }

   

string searchTline=words [getPostionOfLastF(words)+2]; 
string modifyTline=words [getPostionOfLastF(words)+3];

string functionInvo=words[getPostionOfLastF(words)+5]; // a string that has all the function calls


stringstream searchNum(searchTline);
stringstream modifyNum(modifyTline);

string token;

vector<string> stn;

/* parse and get the number of search threads */

while(getline(searchNum,token,':')){
    stn.push_back(token);
}


 numOfSearchT=stoi(stn[1]); //convert it to an int and store it


stn.clear();

/* parse and get the number of modify threads */
while(getline(modifyNum,token,':')){
    stn.push_back(token);
}

 numOfModifyT=stoi(stn[1]);


vector<string> wordsNoF;
    for(int i=0; i<=getPostionOfLastF(words); i++){
        string ss=words[i];
        if(ss.at(0)=='0'||ss.at(0)=='1'|| ss.at(0)=='2'|| ss.at(0)=='3'|| ss.at(0)=='4'|| 
        ss.at(0)=='5'|| ss.at(0)=='6'|| ss.at(0)=='7'|| ss.at(0)=='8'|| 
        ss.at(0)=='9') {
            wordsNoF.push_back(ss);
        }
    }


root=iniTree(wordsNoF,wordsNoF.size()); //construct the inital tree 

/* give the nil node its attributes */
nillNode=new Node(-974810); //arbitrary key for the nil node
 nillNode ->left=nullptr;
 nillNode ->right = nullptr;
 nillNode ->color=BLACK;
 linkNill(root); 
 linkParent(root);
 root ->parent=nillNode;


string trimmed=deleteSpaces(functionInvo);

size_t pos;

ReplaceStringInPlace(trimmed, "||", ",");


stringstream fun (trimmed);
string funToken;

vector <string>searchList;

//fill in the queues 
while (getline(fun,funToken,',')){
    if(funToken.at(0)=='i'||funToken.at(0)=='d'|| funToken.at(0)=='I'||funToken.at(0)=='D'){
        modifyQ.push(funToken);
    }
    else if(funToken.at(0)=='s'||funToken.at(0)=='S'){
        searchList.push_back(funToken);
    }
}


for(int i=0;i<searchList.size();i++){
    searchList[i]=extract(searchList[i],'(',')');
    searchQ.push(stoi(searchList[i]));
}

}

/* a function that traverses the tree to link the leaves to the nil node 
@param node: the starting node (usually the root)
@return void 
*/
void RBT::linkNill(node_t node){
    if(node==nillNode){
        return;
    }
    if(node ->left ==NULL){
        node ->left=nillNode;
    }

    if(node ->right ==NULL){
        node ->right=nillNode;
    }

linkNill(node->left);
linkNill(node->right);

}

/*
A function that traverses the tree to assign the parents of each node after the initial construction of the tree
@param node: the starting node (usually the root)
@return void 
*/
node_t RBT::linkParent(node_t node){
    if(node==nillNode){
        return node;
    }
    node_t lchild=linkParent(node->left);
    lchild->parent=node;

    node_t rchild=linkParent(node->right);
    rchild->parent=node;

return node;

}

/*
The search function for the red black tree with concurrent implemention of the readers/writers implementation (priority with readers)
@param arg: the key
@return void*
*/
void* search (void *arg) {

pthread_mutex_lock(&x); //lock mutex x to ensure that readcount is updated correctly
readcount++; 
if(readcount==1){
    pthread_mutex_lock(&wsem); //if there is one reader, lock wsem to prevent the writers from accessing the shared resource (the tree)to ensure correctness 
}
pthread_mutex_unlock(&x);


//start of critical section
 long item=(long) arg;
 node_t curr=rbt.root;
 node_t parent=nullptr;
 while (curr!=rbt.nillNode && curr->key != item){
     parent=curr;

     if (item < curr->key){
         curr=curr->left;
     }
     else{
         curr = curr ->right;
     }

 }

 if(curr==rbt.nillNode){
     pthread_mutex_lock(&writeSafelyToFile);
     cout<<" search("<<item <<")-> false, performed by thread: "<<(long)pthread_self()<<endl;
     file<<" search("<<item <<")-> false, performed by thread: "<<(long)pthread_self()<<endl;
     pthread_mutex_unlock(&writeSafelyToFile);
 }
  
 else{
   
     pthread_mutex_lock(&writeSafelyToFile);
   cout<<" search("<<item <<")-> true, performed by thread: "<<(long)pthread_self()<<endl;
    file<<" search("<<item <<")-> true, performed by thread: "<<(long)pthread_self()<<endl;
    pthread_mutex_unlock(&writeSafelyToFile);

 }

 //end of critical section
pthread_mutex_lock(&x);
readcount--;
if(readcount==0){
    pthread_mutex_unlock (&wsem);
}

pthread_mutex_unlock(&x);
    
   return  NULL;
}

/* 
A function to do a standard BST insert
@pararm key: the key of the new node
@return void
*/
void RBT::insertHelper (int key){
    node_t newNode=new Node (key);
    node_t y=nillNode;
    node_t x=root;

    while (x!=nillNode){
        y=x;
        if (newNode ->key < x->key){
            x=x->left;
        }
        else{
            x=x->right;
        }
    }
    newNode ->parent=y;
    if(y==nillNode){
        root=newNode;
    }

    else if(newNode -> key < y->key){
        y->left=newNode;
    }
    else{
        y->right=newNode;
    }
    newNode ->left=nillNode;
    newNode ->right=nillNode;
    
    RBInsertFixup (newNode); //fix up the red black tree by possible rotations and recoloring 

}

/*
A function to left rotate the a node in the tree
@pararm: x: the node to be rotated
@return void
*/
void RBT::leftRotate(node_t x){
    node_t y=x ->right;
    x ->right=y->left;
    if(y->left !=nillNode){
        y->left->parent=x;
    }
    y->parent=x->parent;

    if (x->parent ==nillNode){
        root=y;
    }
    else if(x==x->parent->left){
        x->parent->left=y;
    }
    else{
        x->parent->right=y;
    }
    y->left=x;
    x->parent=y;

}

/*
A function to right rotate the a node in the tree
@pararm: x: the node to be rotated
@return void
*/
void RBT::rightRotate(node_t x){
    node_t y=x ->left;
    x ->left=y->right;
    if(y->right !=nillNode){
        y->right->parent=x;
    }
    y->parent=x->parent;

    if (x->parent ==nillNode){
        root=y;
    }
    else if(x==x->parent->right){
        x->parent->right=y;
    }
    else{
        x->parent->left=y;
    }
    y->right=x;
    x->parent=y;

}

/*
A function to help restructure the tree by rotations and recoloring of nodes after inserting a new node so that the tree is a valid red black tree
@param newNode: the node that was just newly inserted to the tree
@return void
*/
void RBT::RBInsertFixup(node_t newNode){
    while (newNode ->parent ->color==RED){

        if(newNode->parent==newNode->parent->parent->left){

            node_t uncle=newNode ->parent->parent->right;
            if(uncle ->color==RED){

                newNode->parent->color=BLACK;
                uncle ->color=BLACK;
                newNode->parent->parent->color=RED;
                newNode=newNode->parent->parent;
            }
            else{
                if(newNode==newNode->parent->right){
                    newNode=newNode->parent;
                    leftRotate (newNode);
                }
                newNode->parent->color=BLACK;
                newNode->parent->parent->color=RED;
                rightRotate(newNode->parent->parent);
            }

        }

     else{
        node_t uncle=newNode ->parent->parent->left;
          if(uncle ->color==RED){
                 newNode->parent->color=BLACK;
                 uncle ->color=BLACK;
                 newNode->parent->parent->color=RED;
                 newNode=newNode->parent->parent;
            }
            else{
                if(newNode==newNode->parent->left){
                    newNode=newNode->parent;
                    rightRotate (newNode);
            }
               newNode->parent->color=BLACK;
                newNode->parent->parent->color=RED;
                leftRotate(newNode ->parent ->parent);
        }
     }
    }
    root->color=BLACK;

}

/*
A helper function to help in the delete of a node in the red black tree
*/
    void RBT::RBTransplant (node_t u, node_t v){
        if (u ->parent ==nillNode){
            root=v;
        }
        else if (u== u ->parent ->left){
            u->parent->left=v;
        }
    else{
        u->parent->right=v;
    }
    v ->parent=u->parent;

    }

/*
The function that deletes a node from the tree
@pararm node:start of the stree (usually the root), key: the key in the node to be deleted 
@return void
*/
void RBT::deleteHelper (node_t node, int key){
    node_t z=nillNode;
    node_t x,y;
    //search that there's a node with the key and locate the node to be deleted 
   while (node != nillNode){
			if (node->key == key) {
				z = node;
			}

			if (node->key <= key) {
				node = node->right;
			} else {
				node = node->left;
			}
		}

		if (z == nillNode) {
			return;
		} 

         y = z;
		int y_original_color = y->color;
		if (z->left == nillNode) {
			x = z->right;
			RBTransplant (z, z->right);
		} else if (z->right == nillNode) {
			x = z->left;
			RBTransplant (z, z->left);
		} else {
            y=z->right;
            while (y ->left !=nillNode){
                y=y->left;
            }
			y_original_color = y->color;
			x = y->right;
			if (y->parent == z) {
				x->parent = y;
			} else {
				RBTransplant (y, y->right);
				y->right = z->right;
				y->right->parent = y;
			}

			RBTransplant (z, y);
			y->left = z->left;
			y->left->parent = y;
			y->color = z->color;
		}
		delete z;
		if (y_original_color == BLACK){
			RBDeleteFixup (x); //fix up the tree after deleting the node 
		}
	}
	
/*
A function to fix up the tree(by recoloring and rotations) and make it a valid red black tree after deleting anode. 
@return void
*/
void RBT::RBDeleteFixup (node_t x){
    node_t s;
		while (x != root && x->color == BLACK) {
			if (x == x->parent->left) {
				s = x->parent->right;
				if (s->color == RED) { //case 1
					s->color = BLACK;
					x->parent->color = RED;
					leftRotate(x->parent);
					s = x->parent->right;
				}

				if (s->left->color == BLACK && s->right->color == BLACK) { //case 2
					s->color = RED;
					x = x->parent;
				} else {
					if (s->right->color == BLACK) { 
						s->left->color = BLACK;
						s->color = RED;
						rightRotate(s);
						s = x->parent->right;
					} 

					s->color = x->parent->color;
					x->parent->color = BLACK;
					s->right->color = BLACK;
					leftRotate(x->parent);
					x = root;
				}
			} else {
				s = x->parent->left;
				if (s->color == RED) { //case 3
					s->color = BLACK;
					x->parent->color = RED;
					rightRotate(x->parent);
					s = x->parent->left;
				}

				if (s->left->color == BLACK && s->right->color == BLACK) {
					s->color = RED;
					x = x->parent;
				} else {
					if (s->left->color == BLACK) {
						s->right->color = BLACK;
						s->color = RED;
						leftRotate(s);
						s = x->parent->left;
					} 

				//case 4
					s->color = x->parent->color;
					x->parent->color = BLACK;
					s->left->color = BLACK;
					rightRotate(x->parent);
					x = root;
				}
			} 
		}
		x->color = BLACK;
	}

/*
The main function to delete a node with ensruing the concurrency part
@param arg: the key
@return void*
*/
void* deleteNode (void *arg){
    long e=(long) arg;

    pthread_mutex_lock(&wsem); //if there's one thread modifying (deleting) from the tree, lock the entire tree
    rbt.deleteHelper(rbt.root,e);
    pthread_mutex_unlock(&wsem); //release the lock after being done


    return NULL;
}

/*
The main function to insert a node with ensruing the concurrency part
@param arg: the key
@return void*
*/
void* insert(void *arg){
        long e=(long) arg;

        pthread_mutex_lock(&wsem); //if there's one thread modifying (inserting) from the tree, lock the entire tree
        rbt.insertHelper(e); 
        pthread_mutex_unlock(&wsem); //release the lock after being done

        return NULL;
    
}

/*
A helper function to trim the spaces in the input file so parsing is easier 
@param str: string that has unneccary spaces
@return str
*/
string RBT::deleteSpaces(string &str){
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    return str;
}


/*
a function to locate the position of the color to help parsing the input file
@pararm: string that has a number and color (e.g. 117r, 98b)
@return postion of the color
*/
int posOfLetter(string str){
    int i;
    for (i=0; i< str.length(); i++){
        if(str.at(i)=='b'|| str.at(i)=='r'){
            break;
        }
    }
    return i;
}

/*
A recursive function to help construct the inital tree in preorder
*/
node_t RBT:: iniTreeHelper (vector <string> elements, int *preIndex, int low, int high, int size){
    // the base case
     if (*preIndex >= size || low > high)  {
        return NULL;
    }

    string str=elements [*preIndex];
    int intNum= stoi(str);
    bool color;
    
    color= (str.at(posOfLetter(str))=='r') ? RED:BLACK;
    node_t root=new Node (intNum,color); 
    *preIndex=*preIndex +1;
    if(low==high){
        return root;
    }
    int i;
    for(i=low; i<=high; i++){ 
        int d=stoi (elements[i]);
        if(d > root->key){
            break;
        }
    }
    root ->left= iniTreeHelper(elements,preIndex,*preIndex,i-1,size);
    root ->right= iniTreeHelper (elements,preIndex,i,high,size);

    return root;

}

/*
The main function to construct the tree from given a preoder with keyes with colors and nilnodes
*/
node_t RBT:: iniTree (vector <string> elements, int size){

int preIndex=0;
return iniTreeHelper(elements,&preIndex,0,size-1,size);

}

/* Helper to get the greatest key in the tree
@param node: the staring node (usually the root)
@return the greatest key
 */
int RBT:: maxKey(node_t node){
    while (node->right!=nillNode){
        node=node->right;
    }
    return node->key;

}

/*
A function to printInPreorder to the output file
@param: the staring node (usually the root)
@return void
*/
void RBT:: printPreOrder (node_t node)  
{  
    if (node == nillNode)  
        return;  
   
    string col= (node ->color==RED) ? "r":"b";
    cout<<node->key<< col <<",";  
    file<<node->key<< col <<",";  
    
    /* output "f" if nil */
     if(node ->left==nillNode){
        cout<<"f,";
        file<<"f,";
    }
     if(node ->right==nillNode){
         if(node->key==largestKey){
             cout<<"f ";
             file<<"f ";
         }
        else{
            cout<<"f,";
            file<<"f,";

        }
    }
    printPreOrder(node->left);  
    printPreOrder(node->right);  
} 

/* A function to determine if a number is a prime to handle some edge cases 
@param n: the number
@return 1 if prime, otherwise 0
*/
int isPrime(int n) 
{ 
    if (n <= 1) 
        return 0; 
  
    for (int i = 2; i < n; i++) 
        if (n % i == 0) 
            return 0; 
  
    return 1; 
}

/* A function to determine the closest divisble number to another number */
int closestDivisible (int n, int m){

    if(isPrime(n)==0){
    while(n%m !=0){
        m++;
    }
}
    return m;
}
/*
 the main method which runs the main thread that does:
1. creates the worker threads (Search and Modify) as specified
2. Let each Search thread picks/removes one search operation from the Readers queue and performs that operation
3. Let each Modify thread picks/removes one insert or delete operation from the Writers queue and performs that operation.
 */
int main(){

pthread_mutex_init(&writeSafelyToFile, NULL); 
pthread_mutex_init(&x, NULL); //dynamically initiliaze the x mutex
pthread_mutex_init(&wsem, NULL); //dynamically initiliaze the wsem mutex


pthread_t searchThreads[numOfSearchT]; 
pthread_t modifyThreads[numOfModifyT];

/* create the search threads and execute the functions as they are being popped from the search queue */

int number=(searchQ.size()%numOfSearchT==0)? searchQ.size()/numOfSearchT : closestDivisible(searchQ.size(),numOfSearchT);

if (numOfSearchT >= searchQ.size()){    
     for(int i=0; i<numOfSearchT; i++){
     if(searchQ.empty()){
             break;
         }
         int element=searchQ.front();
            pthread_create(&searchThreads[i],NULL,search,(void *)element); 
         searchQ.pop();    
    }
}else{
    for(int i=0; i<numOfSearchT; i++){
        int counter=0;
        if(searchQ.empty()){
             break;
         }
        while(counter<number){
            int element=searchQ.front();
            pthread_create(&searchThreads[i],NULL,search,(void *)element); 
            searchQ.pop();
            counter++;
            if(searchQ.empty()){
             break;
         }  
         if(searchQ.empty()){
             break;
         }
     }
    }
}  

/* create the modify threads and execute the functions as they are being popped from the modify queue */
int num2=(modifyQ.size()%numOfModifyT==0)? modifyQ.size()/numOfModifyT : closestDivisible(modifyQ.size(),numOfModifyT);


if (numOfModifyT >= modifyQ.size()){    

for(int i=0; i<numOfModifyT; i++){
        if(modifyQ.empty()){
                break;
        }
        string funCall=modifyQ.front();
        int element =stoi(extract(funCall,'(',')'));
        if(funCall.at(0)=='i'){
            pthread_create(&modifyThreads[i],NULL,insert, (void *)element);
            modifyQ.pop();
        }
        else{
            pthread_create(&modifyThreads[i],NULL,deleteNode, (void *)element);
            modifyQ.pop();
        }
        
}
} else{
    for( int i=0; i<numOfModifyT; i++){
        int counter=0;
        if(modifyQ.empty()){
             break;
         } 
        while(counter <num2){
            string funCall=modifyQ.front();
        int element =stoi(extract(funCall,'(',')'));
        if(funCall.at(0)=='i'){
            pthread_create(&modifyThreads[i],NULL,insert, (void *)element);
            modifyQ.pop();
        }
        else{
            pthread_create(&modifyThreads[i],NULL,deleteNode, (void *)element);
            modifyQ.pop();
        }
        counter++;
        if(modifyQ.empty()){
                 break;
             } 

        }
         if(modifyQ.empty()){
             break;
         } 
    }
}


   
/* wait for the search threads to terminate */
for(int i=0; i<numOfSearchT; i++){
 pthread_join(searchThreads[i],NULL);
}

/* wait for the modify threads to terminate */
for(int i=0; i<numOfModifyT; i++){
 pthread_join(modifyThreads[i],NULL);
}

/*destory the locks after being done */
pthread_mutex_destroy(&writeSafelyToFile);
pthread_mutex_destroy(&x); 
pthread_mutex_destroy (&wsem);


auto end = chrono::steady_clock::now(); //stop the timer
auto diff = end - start;  

cout<<endl;
file<<endl;
cout <<"Execution time(of constructing the tree from the input file then creating threads & executing the functions given in the input file): ";
cout << chrono::duration <double, milli> (diff).count() << " ms" << endl;
file <<"Execution time(of constructing the tree from the input file then creating threads & executing the functions given in the input file): ";
file << chrono::duration <double, milli> (diff).count() << " ms" << endl;

cout<<endl;
file<<endl;

 largestKey=rbt.maxKey(rbt.root);

cout<<"Final RBT: ";
file<<"Final RBT: ";

 rbt.printPreOrder(rbt.root);
cout <<endl;
file <<endl;


file.close(); //close the file after being done with writing 


pthread_exit(NULL); //terminate the calling thread

return 0;
}
