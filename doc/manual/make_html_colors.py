import sys
print "<html>"
print "<body>"
print "<table border=\"0\" cellspacing=\"0\" cellpadding=\"7\"> "

for i in range(16):

    print "<tr >"
    for j in range(4):
        for k in range(4):
            print '<td bgcolor=#',
            if i*8*2 < 16:
                sys.stdout.write( "0" + str((hex(i*8*2)[2:]).upper()))
            else:
                sys.stdout.write( str((hex(i*8*2)[2:]).upper()))
            
            if j*64 < 16: 
                sys.stdout.write( "0" + str((hex(j*64)[2:]).upper())) 
            else:
                sys.stdout.write( str((hex(j*64)[2:]).upper())) 
            if k*64 < 16:
                sys.stdout.write( "0" + str((hex(k*64)[2:]).upper()))
            else:
                sys.stdout.write(  str((hex(k*64)[2:]).upper()))
            sys.stdout.write( ">" ) 


            sys.stdout.write( "0x")
            if i*8*2 < 16:
                sys.stdout.write( "0" + str((hex(i*8*2)[2:]).upper()))
            else:
                sys.stdout.write( str((hex(i*8*2)[2:]).upper()))
            
            if j*64 < 16: 
                sys.stdout.write( "0" + str((hex(j*64)[2:]).upper())) 
            else:
                sys.stdout.write( str((hex(j*64)[2:]).upper())) 
            if k*64 < 16:
                sys.stdout.write( "0" + str((hex(k*64)[2:]).upper()))
            else:
                sys.stdout.write(  str((hex(k*64)[2:]).upper()))


    print "</tr>"
        

print "</table>"
print "</html>"




