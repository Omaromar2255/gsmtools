import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.util.task.ConsoleTaskMonitor;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

public class DecompileAll extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outfile = (args.length > 0) ? args[0] : "DECOMPILED.c";
        String listfile = outfile.replaceAll("\\.c$", "_FUNCTION_LIST.txt");

        println("DecompileAll: output=" + outfile);
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        ConsoleTaskMonitor mon = new ConsoleTaskMonitor();

        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        int total = currentProgram.getFunctionManager().getFunctionCount();
        println("Total functions: " + total);

        BufferedWriter f = new BufferedWriter(new FileWriter(outfile));
        BufferedWriter fl = new BufferedWriter(new FileWriter(listfile));
        f.write("// Ghidra decompiled output for: " + currentProgram.getName() + "\n");
        f.write("// Total functions: " + total + "\n\n");

        int count = 0, failed = 0;
        while (it.hasNext()) {
            if (mon.isCancelled()) break;
            Function fn = it.next();
            String ep = fn.getEntryPoint().toString();
            long size = fn.getBody().getNumAddresses();
            try {
                fl.write(ep + "  " + fn.getName() + "  size=" + size + "\n");
                DecompileResults res = di.decompileFunction(fn, 30, mon);
                if (res != null && res.decompileCompleted()) {
                    f.write("// ==== " + fn.getName() + " @ " + ep + " (size " + size + ") ====\n");
                    f.write(res.getDecompiledFunction().getC());
                    f.write("\n\n");
                } else {
                    failed++;
                    f.write("// DECOMP FAILED " + fn.getName() + " @ " + ep + "\n\n");
                }
            } catch (Exception e) {
                failed++;
                f.write("// EXCEPTION " + fn.getName() + " @ " + ep + ": " + e + "\n\n");
            }
            count++;
            if (count % 100 == 0) {
                println("decompiled " + count + "/" + total);
                f.flush();
            }
        }
        f.close();
        fl.close();
        println("DONE " + count + "/" + total + " (failed=" + failed + ") -> " + outfile);
    }
}
