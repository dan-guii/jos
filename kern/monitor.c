// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#include "kdebug.h"

#define CMDBUF_SIZE	80	// enough for one VGA text line

int mon_show(int argc, char **argv, struct Trapframe *tf);
int mon_si(int argc, char **argv, struct Trapframe *tf);

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// LAB 1: add your command to here...
static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display a listing of function call frames", mon_backtrace },
	{ "show", "Display colorful ASCII art", mon_show },
	{ "si", "Run next instruction and trap back into monitor", mon_si }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// LAB 1: Your code here.
    // HINT 1: use read_ebp().
    // HINT 2: print the current ebp on the first line (not current_ebp[0])
	uint32_t ebp = read_ebp();				// Address of base pointer
	uint32_t eip;							// Address of instruction pointer
	uint32_t args[5];						// Address of arguments
	struct Eipdebuginfo info;

	cprintf("Stack backtrace:\n");
	while (ebp != 0) {										// ebp is originally at 0
		eip = *((uint32_t*) (ebp + 4)); 					// eip is the next instruction after ebp
		cprintf("ebp %x eip %x args", ebp, eip);
		for (int i = 0; i < 5; i++) {						// Loops to get arguments
			args[i] = *((uint32_t*) (ebp + 8 + (i * 4)));	// Add 8 to account for ebp and eip, and 4 for each argument
			cprintf(" %08x ", args[i]);
		}
		cprintf("\n");
		
		int result = debuginfo_eip(eip, &info);
		
		cprintf("%s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);

		ebp = *((uint32_t*) ebp);							// ebp points to address of previous ebp, so cast to pointer and dereference
	}
	return 0;
}

int mon_show(int argc, char **argv, struct Trapframe *tf)
{
    // Sample ASCII art using colors

    cprintf("\x1b[32m░░░░░░░░░░▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄░░░░░░░░░\n");
    cprintf("\x1b[33m░░░░░░░░▄▀░░░░░░░░░░░░▄░░░░░░░▀▄░░░░░░░\n");
    cprintf("\x1b[34m░░░░░░░░█░░▄░░░░▄░░░░░░░░░░░░░░█░░░░░░░\n");
    cprintf("\x1b[35m░░░░░░░░█░░░░░░░░░░░░▄█▄▄░░▄░░░█░▄▄▄░░░\n");
    cprintf("\x1b[36m░▄▄▄▄▄░░█░░░░░░▀░░░░▀█░░▀▄░░░░░█▀▀░██░░\n");
    cprintf("\x1b[32m░██▄▀██▄█░░░▄░░░░░░░██░░░░▀▀▀▀▀░░░░██░░\n");
    cprintf("\x1b[33m░░▀██▄▀██░░░░░░░░▀░██▀░░░░░░░░░░░░░▀██░\n");
    cprintf("\x1b[34m░░░░▀████░▀░░░░▄░░░██░░░▄█░░░░▄░▄█░░██░\n");
    cprintf("\x1b[35m░░░░░░░▀█░░░░▄░░░░░██░░░░▄░░░▄░░▄░░░██░\n");
    cprintf("\x1b[36m░░░░░░░▄█▄░░░░░░░░░░░▀▄░░▀▀▀▀▀▀▀▀░░▄▀░░\n");
	cprintf("\x1b[31m░░░░░░█▀▀█████████▀▀▀▀████████████▀░░░░\n");
    cprintf("\x1b[32m░░░░░░████▀░░███▀░░░░░░▀███░░▀██▀░░░░░░\n");
    cprintf("\x1b[33m░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\x1b[0m\n");

    return 0;
}

int mon_si(int argc, char **argv, struct Trapframe *tf) {
    if (!tf) {
        cprintf("No trap frame available.\n");
        return 0;
    }

    tf->tf_eflags |= FL_TF;		// Single Step Trap Flag
    return -1; 
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}