package edu.cs300;

import CtCILibrary.*;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map; 
import java.util.concurrent.*;

class Worker extends Thread{

  Trie textTrieTree;
  ArrayBlockingQueue prefixRequestArray;
  ArrayBlockingQueue resultsOutputArray;
  int id;
  String passageName;
  int totalPassages;

  public Worker(String[] words,int id, ArrayBlockingQueue prefix, ArrayBlockingQueue results, String passageName, int totalPassages){
    this.textTrieTree=new Trie(words);
    //System.out.println("Length of words arr in worker: " + words.length);
    this.prefixRequestArray=prefix;
    this.resultsOutputArray=results;
    this.id=id;
    this.passageName=passageName; //put name of passage here
    this.totalPassages = totalPassages;
  }

  public void run() {
    System.out.println("Worker-"+this.id+" ("+this.passageName+") thread started ...");
    //while (true){
      try {
        SearchRequest prefix=(SearchRequest)this.prefixRequestArray.take();
        boolean found = this.textTrieTree.contains(prefix.prefix);
        
        
        if (!found)
        {
          System.out.println("Worker-"+this.id+" "+prefix.requestID+":"+ prefix+" ==> not found ");
          //System.out.println("msgsnd Reply " + this.id + " of " + this.totalPassages + " on " + prefix.requestID + ":" + prefix.prefix +
          //                   " from " + this.passageName + " present=0 lw=----(len=4) msglen=144");

          MessageJNI.writeLongestWordResponseMsg(prefix.requestID, prefix.prefix, this.id, this.passageName, null, this.totalPassages, 0);
        } 
        else
        {
          //To find the longest word given a prefix, simply find the longest word in trie with the navigated prefix node as root
          //ie. given 'con' prefix, start at node c->o->n->{start of BFS}
          TrieNode prefixRoot = this.textTrieTree.getRoot();
          for(char character : prefix.prefix.toCharArray())
          {
            prefixRoot = prefixRoot.getChild(character);
          }

          //NOTE: Had to recompile CtClLibrary to create get function for children hashmap
          //This is a simple BFS of the trie with updated root, while populating a hashmap with a node, and the node that discovered them
          LinkedList<TrieNode> queue = new LinkedList();
          Map<TrieNode, TrieNode> bfsMap = new HashMap<TrieNode, TrieNode>();
          queue.push(prefixRoot);
          TrieNode currentNode = null;
          TrieNode last = null;
          while(!queue.isEmpty())
          {
            currentNode = queue.pop();
            if(!currentNode.terminates())
            {
              //Loop through all children of current operating node
              for(TrieNode children : currentNode.getChildren().values())
              {
                //Add children to queue
                queue.push(children);
                //Mark each child as having a parent current node, useful for later BFS navigation
                bfsMap.put(children, currentNode);
              }
            }
            last = currentNode;
          }

          currentNode = last;

          //Now we iterate over the produced bfs path and produce our word!
          String longestWord = "";
          while(currentNode != null)
          {
            longestWord = currentNode.getChar() + longestWord;
            currentNode = bfsMap.get(currentNode);
          }

          //This just adds the prefix to the discovered longest word
          longestWord = prefix.prefix.substring(0, prefix.prefix.length()-1) + longestWord;

          System.out.println("Worker-"+this.id+" "+prefix.requestID+":"+ prefix+" ==> " + longestWord);
          //System.out.println("msgsnd Reply " + this.id + " of " + this.totalPassages + " on " + prefix.requestID + ":" + prefix.prefix +
          //                   " from " + this.passageName + " present=0 lw=" + longestWord + "(len=" + longestWord.length() + ") msglen=144");

          MessageJNI.writeLongestWordResponseMsg(prefix.requestID, prefix.prefix, this.id, this.passageName, null, this.totalPassages, 0);
        }
      } catch(InterruptedException e){
        System.out.println(e.getMessage());
      }
    //}
  }

}
