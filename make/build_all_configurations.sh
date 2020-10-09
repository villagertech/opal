#!/bin/bash

outfile="$1"
if [ -z "$outfile" ]; then
   echo usage: "`basename $0` outfile [ <start> ]"
   exit 1
fi

if [ -z "$PTLIBDIR" ]; then
   if [ ! -d ptlib ]; then
       echo "No PTLIBDIR set."
       exit 1
   fi
   export PTLIBDIR=`pwd`/ptlib
fi

if [ -z "$OPALDIR" ]; then
   if [ ! -d opal ]; then
       echo "No OPALDIR set."
       exit 1
   fi
   export OPALDIR=`pwd`/opal
fi

PTLIB_OPTS=`$PTLIBDIR/configure --help | \
          sed --silent --regexp-extended \
              --expression=s/--disable-FEATURE// \
              --expression="s/^  (--disable-[^ ]+).*$/\\1/p"`

OPAL_OPTS=`$OPALDIR/configure --help | \
          sed --silent --regexp-extended \
              --expression=s/--disable-FEATURE// \
              --expression="s/^  (--disable-[^ ]+).*$/\\1/p"`

if [ -n "$2" ]; then
    skipping="--disable-$2"
    echo "Skipping builds until $skipping"
fi

echo $0                                                               > $outfile
for opt in $PTLIB_OPTS ; do
   echo =========================================================    >> $outfile
   if [ -n "$skipping" -a "$skipping" != "$opt" ]; then
      echo Skipping $opt                                       | tee -a $outfile
      continue
   fi
   skipping=""
   echo Trying $opt                                            | tee -a $outfile
   echo ---------------------------------------------------------    >> $outfile
   rm -rf $PTLIBDIR/lib_* $PTLIBDIR/samples/*/obj_* $OPALDIR/lib_* $OPALDIR/samples/*/obj_*
   echo ---------------------------------------------------------    >> $outfile
   make -C $PTLIBDIR CONFIG_PARMS="$opt --enable-samples" config     >> $outfile 2>&1
   if [ $? -ne 0 ]; then
      echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                    >> $outfile
      echo "!!!!! PTLib CONFIG FAILED for $opt"                | tee -a $outfile
      echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                    >> $outfile
   else
      echo --------------------------------------------------------- >> $outfile
      if [ "$opt"=="--disable-url" -o "$opt"=="--disable-http" ]; then
	 make -C $PTLIBDIR all                                       >> $outfile 2>&1
      else
	 make -C $OPALDIR config                                     >> $outfile 2>&1
	 if [ $? -ne 0 ]; then
	    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"              >> $outfile
	    echo "!!!!! OPAL CONFIG FAILED for $opt"           | tee -a $outfile
	    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"              >> $outfile
	 else
	    make -C $OPALDIR all                                     >> $outfile 2>&1
	    if [ $? -ne 0 ]; then
	       echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"            >> $outfile
	       echo "!!!!! BUILD FAILED for $opt"              | tee -a $outfile
	       echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"            >> $outfile
	    fi
	 fi
      fi
   fi
done

echo =========================================================       >> $outfile
echo Restoring full options                                    | tee -a $outfile
echo ---------------------------------------------------------       >> $outfile
rm -rf $PTLIBDIR/lib_* $PTLIBDIR/samples/*/obj_*
echo ---------------------------------------------------------       >> $outfile
make -C $PTLIBDIR  CONFIG_PARMS="--enable-samples" config            >> $outfile 2>&1
echo ---------------------------------------------------------       >> $outfile
make -C $PTLIBDIR all                                                >> $outfile 2>&1

for opt in $OPAL_OPTS ; do
   echo =========================================================    >> $outfile
   if [ -n "$skipping" -a "$skipping" != "$opt" ]; then
      echo Skipping $opt                                       | tee -a $outfile
      continue
   fi
   skipping=""
   echo Trying $opt                                            | tee -a $outfile
   echo ---------------------------------------------------------    >> $outfile
   rm -rf $OPALDIR/lib_* $OPALDIR/samples/*/obj_*
   echo ---------------------------------------------------------    >> $outfile
   make -C $OPALDIR CONFIG_PARMS="$opt --enable-samples" config      >> $outfile 2>&1
   if [ $? -ne 0 ]; then
      echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                    >> $outfile
      echo "!!!!! OPAL CONFIG FAILED for $opt"                 | tee -a $outfile
      echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                    >> $outfile
   else
      echo --------------------------------------------------------- >> $outfile
      make -C $OPALDIR all                                           >> $outfile 2>&1
      if [ $? -ne 0 ]; then
	 echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                  >> $outfile
	 echo "!!!!! BUILD FAILED for $opt"                    | tee -a $outfile
	 echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"                  >> $outfile
      fi
   fi
done

echo =========================================================       >> $outfile
echo "Completed building all possible configurations."         | tee -a $outfile

