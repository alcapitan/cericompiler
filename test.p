[i,j,a,b,c]

FOR i := 1 TO 10 DO
    FOR j := 1 TO 10 DO
        ;
    ENDFOR
    a := i + j;
    PRINT a;
ENDFOR

a := -3;
b := 7;
IF (a < b) THEN
    b := -2;
ELSE
    b := -6;
ENDIF
PRINT b;

c := 2;
WHILE (c < 4096) DO
  c := c * 2;
  PRINT c;
ENDWHILE

.