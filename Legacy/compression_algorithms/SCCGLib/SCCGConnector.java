 
import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import SCCG.SCCGC;
import SCCG.SCCGD;

public class SCCGConnector {

    public static void main(String args[]) throws IOException, InterruptedException {
        BufferedReader buffReader = new BufferedReader(new InputStreamReader(System.in));
        String line;
        String[] compressArgs;
        System.out.println(System.getProperty("user.dir") + "");
        do {
            line = buffReader.readLine();
            if (line == null) {
                break;
            }
            if (line.startsWith("END"))
                break;
            //Split line by whitespace, we trim so that any leading whitespace is not included
            //The original string may have quotes to include spaces in the arguments. We should
            //respect those quotes
            Matcher m = Pattern.compile("([^\"]\\S*|\".+?\")\\s*").matcher(line.trim());
            List<String> argz = new ArrayList<String>(3); //Expected 3 args
            //compressArgs = line.trim().split("\\s+");
            while (m.find()) {
                argz.add(m.group(1).replace("\"", ""));
            }
            //System.out.println(argz);
            //Array for the arguments, but without the first (c/d)
            compressArgs = new String[argz.size() - 1];
            for (int i = 0; i < compressArgs.length; i++) {
                compressArgs[i] = argz.get(i+1);
            }
            try {
                if (argz.get(0).equals("c")) {
                    SCCGC.main(compressArgs);
                } else if (argz.get(0).equals("d")) {
                    SCCGD.main(compressArgs);
                } else {
                    System.out.println("UNKNOWN COMMAND");
                }
            } catch (Exception e) {
                System.out.println(e);
            } finally {
                System.out.println("RETURN");
            }
        } while (true);
    }
}
