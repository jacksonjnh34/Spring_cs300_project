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

    public static void main(String[] args)
    {
        //Does the file path point to root directory or what?
        File inputPassages = new File("passages.txt");
        ArrayList<String[]> passageWords = new ArrayList<String[]>();

        ArrayBlockingQueue prefixRequestArray;
        ArrayBlockingQueue resultsOutputArray;

        Scanner scan;
        try 
        {
            scan = new Scanner(inputPassages);
        } catch (FileNotFoundException e) 
        {
            System.out.println("No input passages file found at: " + inputPassages);
            e.printStackTrace();
            return;
        }

        while(scan.hasNextLine())
        {
            String passageLocation = scan.nextLine();

            if(passageLocation.length() > 1)
            {
                //2 Questions:
                //How to handle punctuation? Do we just strip it?
                //Do we convert to lower case (Assume yes)
                Scanner wordScan;
                try 
                {
                    wordScan = new Scanner(new File(passageLocation));
                } catch (FileNotFoundException e) 
                {
                    System.out.println("No passage file found at: " + passageLocation);
                    e.printStackTrace();
                    return;
                }
                
                List<String> lines = new ArrayList<String>();
                while(wordScan.hasNextLine())
                {
                    lines.add(wordScan.nextLine().replaceAll("[^a-zA-Z ]", "").toLowerCase());
                }

                String[] words = lines.toArray(new String[0]);
                passageWords.add(words);
            }

        }

        scan.close();

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
}