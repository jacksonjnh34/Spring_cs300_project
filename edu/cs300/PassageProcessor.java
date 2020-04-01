package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.concurrent.*;
import edu.cs300.MessageJNI;
import edu.cs300.SearchRequest;

public class PassageProcessor {

    public static void main(String[] args) {
        // Initialize/Declare data structures for later
        File inputPassages = new File("passages.txt");
        ArrayList<String[]> passageWords = new ArrayList<String[]>();
        List<String> passages = new ArrayList<String>();

        // Begin reading in the file paths for the passages from passages.txt located in
        // root
        Scanner scan;
        try {
            scan = new Scanner(inputPassages);
        } catch (FileNotFoundException e) {
            // If file is not found, exit
            System.out.println("No input passages file found at: " + inputPassages);
            e.printStackTrace();
            return;
        }

        // Begin processing each individual passage and compile into String array of
        // discrete words
        while (scan.hasNextLine()) {
            String passageLocation = scan.nextLine();
            passages.add(passageLocation);

            if (passageLocation.length() > 1) {
                //Start scanning in words
                Scanner wordScan;
                try {
                    wordScan = new Scanner(new File(passageLocation));
                } catch (FileNotFoundException e) {
                    // If file is not found, exit
                    System.out.println("No passage file found at: " + passageLocation);
                    e.printStackTrace();
                    return;
                }

                // Read in each line of file (allows for multi-line passage processing)
                List<String> lines = new ArrayList<String>();
                while (wordScan.hasNext()) {
                    // Strip punctuation from text and convert all chars to lower case for coherent
                    // trie constructiion. Also remove all contractions from the array
                    lines.add(wordScan.next().replaceAll("(\\w*'\\w*|\\w*'\\w*)", "").replaceAll("[^a-zA-Z ]", "").toLowerCase());
                }

                // Convert lines ArrayList into a simple array of words, then add to the
                // ArrayList of passageWords
                String[] words = lines.toArray(new String[0]);
                passageWords.add(words);
            }
        }
        // Fucntionally, this is just a preprocessing step so that we don't have to
        // process each passage whenever a new worker thread is created

        scan.close();

        // Is there an optimal value for the capacity of this? .put() just waits to add
        // to queue if full but still
        ArrayBlockingQueue[] prefixRequestArrays = new ArrayBlockingQueue[passageWords.size()];
        ArrayBlockingQueue<String> resultsOutputArray = new ArrayBlockingQueue<String>(passageWords.size() * 10);

        for (int i = 0; i < passageWords.size(); i++) {
            //Initialize the ArrayBlockingQueues for each to be created worker thread
            prefixRequestArrays[i] = new ArrayBlockingQueue<SearchRequest>(passageWords.size());
        }

        for (int i = 0; i < passageWords.size(); i++) {
            //Create a worker thread for each passage
            new Worker(passageWords.get(i), i, prefixRequestArrays[i], resultsOutputArray, passages.get(i),
                    passageWords.size()).start();
        }

        //Create a while loop to execute until prefix with ID=0 is recieved
        boolean managerExit = false;
        while (!managerExit) {
            //Read in prefix requests from SystemV message queue            
            SearchRequest request = MessageJNI.readPrefixRequestMsg();

            //Print out that the prefix has been recieved
            System.out.print("**prefix(" + request.requestID + ") " + request.prefix + " received\n");

            //Set to exit loop on id=0
            if (request.requestID == 0) 
            {
                managerExit = true;
            } 
            else 
            {
                //Try to place the prefix onto the ArrayBlockingQueue for the workers to take off and process
                try {
                    for (int i = 0; i < passageWords.size(); i++) {
                        prefixRequestArrays[i].put(request);
                    }
                } catch (InterruptedException e) {}

                //Write to the SystemV message queue for each passage after all the messages have been recieved
                for (int i = 0; i < passageWords.size(); i++) {
                    String output[];
                    try 
                    {
                        output = resultsOutputArray.take().split(":");
                        MessageJNI.writeLongestWordResponseMsg(Integer.parseInt(output[0]), output[1], Integer.parseInt(output[2]), output[3], output[4], 
                                                           Integer.parseInt(output[5]), Integer.parseInt(output[6]));
                    } catch (InterruptedException e) 
                    {
                        output = null;
                    }
                }
            }
            
        }

        //Exit and terminate program
        System.out.println("Terminating...");
        System.exit(0);
    }
}
