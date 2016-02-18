/*****************************************************************************
This was lifted from the GRUB source code.
GRUB is available under the GNU general public license from
	http://www.gnu.org/software/grub
	ftp://alpha.gnu.org/gnu/grub
*****************************************************************************/
#include "mltiboot.h"
#include <stdio.h>

static int quiet;
/*****************************************************************************
*****************************************************************************/
static int
check_multiboot (const char *filename, FILE *fp)
{
  multiboot_header_t *mbh = 0;
  int i;
  char buf[8192];

  if (fread (buf, 1, 8192, fp) < 0)
    {
      fprintf (stderr, "%s: Read error.\n", filename);
      return 0;
    }

  for (i = 0; i < 8192 - sizeof (multiboot_header_t); i++)
    {
      unsigned long magic = *((unsigned long *) (buf + i));

      if (magic == MULTIBOOT_HEADER_MAGIC)
	{
	  mbh = (multiboot_header_t *) (buf + i);
	  break;
	}
    }

  if (! mbh)
    {
      fprintf (stderr, "%s: No Multiboot header.\n", filename);
      return 0;
    }

  if (! quiet)
    printf ("%s: The Multiboot header is found at the offset %d.\n",
	    filename, i);

  /* Check for the checksum.  */
  if (mbh->magic + mbh->flags + mbh->checksum != 0)
    {
      fprintf (stderr,
	       "%s: Bad checksum (0x%lx).\n",
	       filename, mbh->checksum);
      return 0;
    }

  /* Reserved flags must be zero.  */
  if (mbh->flags & ~0x00010003)
    {
      fprintf (stderr,
	       "%s: Non-zero is found in reserved flags (0x%lx).\n",
	       filename, mbh->flags);
      return 0;
    }

  if (! quiet)
    {
      printf ("%s: Page alignment is turned %s.\n",
	      filename, (mbh->flags & 0x1)? "on" : "off");
      printf ("%s: Memory information is turned %s.\n",
	      filename, (mbh->flags & 0x2)? "on" : "off");
      printf ("%s: Address fields is turned %s.\n",
	      filename, (mbh->flags & 0x10000)? "on" : "off");
    }

  /* Check for the address fields.  */
  if (mbh->flags & 0x10000)
    {

printf("header_addr = 0x%lX\n", mbh->header_addr);
printf("load_addr = 0x%lX\n", mbh->load_addr);
printf("load_end_addr = 0x%lX\n", mbh->load_end_addr);
printf("bss_end_addr = 0x%lX\n", mbh->bss_end_addr);
printf("entry_addr = 0x%lX\n", mbh->entry_addr);

      if (mbh->header_addr < mbh->load_addr)
	{
	  fprintf (stderr,
		   "%s: header_addr is less than "
		   "load_addr (0x%lx > 0x%lx).\n",
		   filename, mbh->header_addr, mbh->load_addr);
	  return 0;
	}

      if (mbh->load_addr >= mbh->load_end_addr)
	{
	  fprintf (stderr,
		   "%s: load_addr is not less than load_end_addr"
		   " (0x%lx >= 0x%lx).\n",
		   filename, mbh->load_addr, mbh->load_end_addr);
	  return 0;
	}

      if (mbh->load_end_addr > mbh->bss_end_addr)
	{
	  fprintf (stderr,
		   "%s: load_end_addr is greater than bss_end_addr"
		   " (0x%lx > 0x%lx).\n",
		   filename, mbh->load_end_addr, mbh->bss_end_addr);
	  return 0;
	}

      if (mbh->load_addr > mbh->entry_addr)
	{
	  fprintf (stderr,
		   "%s: load_addr is greater than entry_addr"
		   " (0x%lx > 0x%lx).\n",
		   filename, mbh->load_addr, mbh->entry_addr);
	  return 0;
	}

      if (mbh->load_end_addr <= mbh->entry_addr)
	{
	  fprintf (stderr,
		   "%s: load_end_addr is not less than entry_addr"
		   " (0x%lx <= 0x%lx).\n",
		   filename, mbh->load_end_addr, mbh->entry_addr);
	  return 0;
	}

      /* This is a GRUB-specific limitation.  */
      if (mbh->load_addr < 0x100000)
	{
	  fprintf (stderr,
		   "%s: Cannot be loaded at less than 1MB by GRUB"
		   " (0x%lx).\n",
		   filename, mbh->load_addr);
	  return 0;
	}
    }

  if (! quiet)
    printf ("%s: All checks passed.\n", filename);

  return 1;
}
/*****************************************************************************
*****************************************************************************/
int main(unsigned arg_c, char *arg_v[])
{
	FILE *file;

	if(arg_c < 2)
	{
		printf("Checks if file is Multiboot compatible\n");
		return 1;
	}
	file = fopen(arg_v[1], "rb");
	if(file == NULL)
	{
		printf("Can't open file '%s'\n", arg_v[1]);
		return 2;
	}
	check_multiboot(arg_v[1], file);
	fclose(file);
	return 0;
}
