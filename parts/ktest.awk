BEGIN {
  n=300;
  printf("S100 v0 w0 a10 t 0 .25 0 0 l1\n");
}
{
  printf("{%s} /ks wait 50 k>d d>r /r %d\n", $1, n++);
}
END {
  for (i=300; i<n; i++) {
    printf("v0 w%d l1 wait 500\n", i);
  }
  printf("v0 w0 l1\n");
}
