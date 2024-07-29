extern void io_hlt(void);

void
hari_main(void)
{
fin:
    io_hlt();
    goto fin;
}
