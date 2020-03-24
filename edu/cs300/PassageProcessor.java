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
                // 2 Questions:
                // How to handle punctuation? Do we just strip it?
                // Do we convert to lower case (Assume yes)
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
                    // trie constructiion
                    lines.add(wordScan.next().replaceAll("(\\w*'\\w+|\\w+'\\w*)", "").replaceAll("[^a-zA-Z ]", "")
                            .toLowerCase());
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
            prefixRequestArrays[i] = new ArrayBlockingQueue<SearchRequest>(passageWords.size());
            /*
             * try { prefixRequestArrays[i].put(new SearchRequest(1, "pre")); } catch
             * (InterruptedException e) {}
             */
        }

        for (int i = 0; i < passageWords.size(); i++) {
            new Worker(passageWords.get(i), i, prefixRequestArrays[i], resultsOutputArray, passages.get(i),
                    passageWords.size()).start();
        }

        int resultsArrSizeTemp = resultsOutputArray.size();
        boolean managerExit = false;
        while (!managerExit) {
            // System.out.println("READ BUFFER");
            
            SearchRequest request = MessageJNI.readPrefixRequestMsg();

            System.out.println("**prefix(" + request.requestID + ") " + request.prefix + " received");

            if (request.requestID == 0) 
            {
                managerExit = true;
            } 
            else 
            {
                try {
                    for (int i = 0; i < passageWords.size(); i++) {
                        prefixRequestArrays[i].put(request);
                    }
                } catch (InterruptedException e) {}

                
                for (int i = 0; i < passageWords.size(); i++) {
                    String output[];
                    try 
                    {
                        output = resultsOutputArray.take().split(":");
                        System.out.println(Integer.parseInt(output[0]) + ":" + output[1] + ":" + Integer.parseInt(output[2]) + ":" + output[3] + ":" + output[4] + ":" + 
                                           Integer.parseInt(output[5]) + ":" + Integer.parseInt(output[6]));
                        MessageJNI.writeLongestWordResponseMsg(Integer.parseInt(output[0]), output[1], Integer.parseInt(output[2]), output[3], output[4], 
                                                           Integer.parseInt(output[5]), Integer.parseInt(output[6]));
                    } catch (InterruptedException e) 
                    {
                        output = null;
                    }
                }
            }
            
        }

        System.out.println("Terminating...");
        System.exit(0);
    }
      

/*
        for(int i = 0; i < passageWords.size(); i++)
        {
            System.out.println("PASSAGE " + i+1 + ": ");
            for(int j = 0; j < passageWords.get(i).length; j++)
            {
                System.out.print(passageWords.get(i)[j] + " ");
            }
            System.out.println();
        }
*/

}
