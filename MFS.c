/*
  csc360 assignment2
  Yaxi Yu
  V00828218
  topic: Multi-Flow Scheduler (MFS)
*/

//======================includes===================================================
#include <stdio.h>     //printf(), scanf() setuf(), perror(),popen(),fgets()
#include <stdlib.h>    //malloc(),exit()
#include <pthread.h>
#include <errno.h>     //errors
#include <unistd.h>   //usleep
#include <stdbool.h>  //boolean
#include <string.h>
struct timeval start;


//===============data type and global constants===================================
	/*
	*pthread_mutex_t myMutex;   //mutex declaration
	*pthread_mutex_init(myMutex,	myAttr);	(the second arg can be NULL)	
	*pthread_mutex_destroy(myMutex);	
	*pthread_mutex_lock(myMutex);	
	*pthread_mutex_unlock(myMutex);	
	*pthread_mutex_trylock(myMutex);	
	*/
	/*
	*pthread_cond_t myConvar;   //condition variable
	* pthread_cond_init(myConvar,	myAttr); (myAttr is usually ignored and specified as NULL)
	* pthread_cond_destroy(myConvar);	
	* pthread_cond_wait(myConvar,	myMutex);	
	* pthread_cond_signal(myConvar);	
	* pthread_cond_broadcast(myConvar);	
	*/

typedef struct _flow{
	int id;
	float arrivalTime;
	float transTime;
	int priority;
}flow;

#define MAXFLOW 100

flow flowList[MAXFLOW];   // parse input in an array of flow
flow *queueList[MAXFLOW];  // store waiting flows while transmission pipe is occupied.
pthread_t thrList[MAXFLOW]; // each thread executes one flow
pthread_mutex_t trans_mtx = PTHREAD_MUTEX_INITIALIZER ; 
pthread_cond_t trans_cvar = PTHREAD_COND_INITIALIZER ;  
bool pipeok=true;
int numInQueue;
int idInPipe;
struct timeval start_time;

/*
inputfile example:
3
1:3,60,3
2:6,70,1
3:3,50,3
*/


//=======================functions==================================================
double relative_time(){
	struct timeval curr;
	gettimeofday(&curr,NULL);
	double time = (curr.tv_sec - start.tv_sec)+((curr.tv_usec - start.tv_usec)/1000000.0);
	return time;
}
void requestPipe(flow *item) {

    int i;
    //lock mutex;
    pthread_mutex_lock(&trans_mtx);



   
    //when there's nothing in the pipe nor the queue
    if( numInQueue==0 && pipeok==true){
       //close the pipe 
       pipeok=false; 
       //start running this item
       idInPipe=item->id;
       //unlock mutex
       pthread_mutex_unlock(&trans_mtx);
       return;
    }


    //add item in queue, sort the queue according rules
    //if the queue is empty
    if(numInQueue==0){
        queueList[0]=item;
    }else{

    //if the queue is not empty, put the item at the end of the queueList
    
    queueList[numInQueue]=item;

    //determine priority swap the item 
    //in the queue list, determine the trasmission time 
    i = numInQueue;
    flow *tmp;
    while(i>0){
	//start priority check
	if(queueList[i-1]->priority > queueList[i]->priority){
		tmp = queueList[i-1];
		queueList[i-1] = queueList[i];
		queueList[i] = tmp;
	}else if (queueList[i-1]->priority == queueList[i]->priority){
		//start arrival time check
		if(queueList[i-1]->arrivalTime > queueList[i]->arrivalTime){
			tmp = queueList[i-1];
			queueList[i-1] = queueList[i];
			queueList[i] = tmp;
		}else if (queueList[i-1]->arrivalTime == queueList[i]->arrivalTime){
				//start trans time check
				if(queueList[i-1]->transTime > queueList[i]->transTime){
					tmp = queueList[i-1];
					queueList[i-1] = queueList[i];
					queueList[i] = tmp;
				}
				else if(queueList[i-1]->transTime == queueList[i]->transTime)
				{
					//start id check
					if(queueList[i-1]->id > queueList[i]->id)
					{
						tmp = queueList[i-1];
						queueList[i-1] = queueList[i];
						queueList[i] = tmp;
					}
				}
			}

		}

		i--;
	}
	}
	int j;
	/*for(j=0; j<numInQueue; j++){
		printf("here is the list: %d %d\n",j,queueList[j]->id);
	}*/

    numInQueue++;

    /*
    for(i=numInQueue-1; i>=1; i--){
         //if same priority
    	 if(queueList[i]->priority==queueList[i-1]->priority){
               if(queueList[i]->transTime == queueList[i-1]->transTime){
                      if(queueList[i]->arrivalTime == queueList[i-1]->arrivalTime){
                            if(queueList[i]->id < queueList[i-1]->id){
				   flow *temp=queueList[i-1];
				   queueList[i-1] = queueList[i];
				   queueList[i] = temp;                

                            }
 
                      }else if(queueList[i]->arrivalTime < queueList[i-1]->arrivalTime){
				   flow *temp=queueList[i-1];
				   queueList[i-1] = queueList[i];
				   queueList[i] = temp;              
                      }


               }
         }else if(queueList[i]->priority>queueList[i-1]->priority){
		   flow *temp=queueList[i-1];
		   queueList[i-1] = queueList[i];
		   queueList[i] = temp;   

         }
    }
    */
    printf("Flow %2d waits for the finish of flow %2d. \n",item->id, idInPipe);


   
    //wait till pipe to be available and be at the top of the queue
    //while the item is not at the front of the queuelist

    while(item->id != idInPipe){
             pthread_cond_wait(&trans_cvar, &trans_mtx);
             idInPipe=queueList[0]->id;             
    }
    
    numInQueue--;
    if(numInQueue==0){
        idInPipe=0;
    }
    // update queue
    for (i=0; i<numInQueue; i++){
         queueList[i]=queueList[i+1];
    }
    queueList[numInQueue]=NULL;
   
    //unlock mutex;

}

void releasePipe() {
 
    pthread_cond_broadcast(&trans_cvar);
    pthread_mutex_unlock(&trans_mtx);
   
}




// entry point for each thread created
void *thrFunction(void *flowItem) {

    flow *item = (flow *)flowItem ;

    // wait for arrival
    usleep(item->arrivalTime*1000000);
    printf("Flow %2d arrives: arrival time (%.2f), transmission time (%.1f), priority (%2d). \n",item->id,item->arrivalTime,item->transTime,item->priority);

    // wait for pipe and start transmission
    requestPipe(item) ;
    printf("Flow %2d starts its transmission at time %.2f. \n",item->id,relative_time());

    // sleep for transmission time
    usleep(item->transTime*1000000);

    // finish
    releasePipe(item) ;

    printf("Flow %2d finishes its transmission at time %.2f. \n",item->id,relative_time());

}




//========================main==========================================================
int main(int argc, char **argv){
        numInQueue=0;

	//file handling 
	FILE *file = fopen(argv[1],"r");
	if(file == NULL){
		printf("can't open file\n");
		return 1;
	}
	

        // read number of flows
        int num_flow;
        fscanf(file, "%d\n", &num_flow);       
          
        // read each flow
	gettimeofday(&start, NULL);
	int i;
	float arrive,trans;
        for(i=0; i<num_flow; i++) {
             if(!(fscanf(file,"%d:%f,%f,%d\n",&flowList[i].id,&arrive,&trans,&flowList[i].priority))){
			printf("wrong input format. ");
		}
		flowList[i].arrivalTime=arrive/10;
		flowList[i].transTime=trans/10;
        }
  

        // release file descriptor
        fclose(file);
   
 

        // create a thread for each flow
        int z;
        for(z=0;z<num_flow;z++){ 
            pthread_create(&thrList[z], NULL, thrFunction, (void *)&flowList[z]);
        }

        // wait for all threads to terminate
        for(z=0;z<num_flow;z++){ 
            pthread_join(thrList[z], NULL);
        }

        // destroy mutex & condition variable
	pthread_mutex_destroy(&trans_mtx);
	pthread_cond_destroy(&trans_cvar);
        return 0;	

}
//===========================end==========================================================



