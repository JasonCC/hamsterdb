
all:de/crupp/hamsterdb/Const.class \
        de/crupp/hamsterdb/DatabaseException.class \
        de/crupp/hamsterdb/Transaction.class \
        de/crupp/hamsterdb/Database.class \
        de/crupp/hamsterdb/Environment.class \
        de/crupp/hamsterdb/Cursor.class \
        de/crupp/hamsterdb/Version.class \
        de/crupp/hamsterdb/License.class \
        de/crupp/hamsterdb/Parameter.class \
        de/crupp/hamsterdb/ErrorHandler.class \
        de/crupp/hamsterdb/CompareCallback.class \
        de/crupp/hamsterdb/PrefixCompareCallback.class \
        de/crupp/hamsterdb/DuplicateCompareCallback.class \
        jar

jar:
	$(JDK)/bin/jar -cf hamsterdb-$(PACKAGE_VERSION).jar \
		de/crupp/hamsterdb/*.class 

de/crupp/hamsterdb/Const.class: de/crupp/hamsterdb/Const.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Const.java

de/crupp/hamsterdb/DatabaseException.class: de/crupp/hamsterdb/DatabaseException.java
	$(JDK)/bin/javac de/crupp/hamsterdb/DatabaseException.java

de/crupp/hamsterdb/Database.class: de/crupp/hamsterdb/Database.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Database.java

de/crupp/hamsterdb/Environment.class: de/crupp/hamsterdb/Environment.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Environment.java

de/crupp/hamsterdb/Cursor.class: de/crupp/hamsterdb/Cursor.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Cursor.java

de/crupp/hamsterdb/Version.class: de/crupp/hamsterdb/Version.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Version.java

de/crupp/hamsterdb/License.class: de/crupp/hamsterdb/License.java
	$(JDK)/bin/javac de/crupp/hamsterdb/License.java

de/crupp/hamsterdb/Parameter.class: de/crupp/hamsterdb/Parameter.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Parameter.java

de/crupp/hamsterdb/ErrorHandler.class: de/crupp/hamsterdb/ErrorHandler.java
	$(JDK)/bin/javac de/crupp/hamsterdb/ErrorHandler.java

de/crupp/hamsterdb/CompareCallback.class: de/crupp/hamsterdb/CompareCallback.java
	$(JDK)/bin/javac de/crupp/hamsterdb/CompareCallback.java

de/crupp/hamsterdb/PrefixCompareCallback.class: de/crupp/hamsterdb/PrefixCompareCallback.java
	$(JDK)/bin/javac de/crupp/hamsterdb/PrefixCompareCallback.java

de/crupp/hamsterdb/DuplicateCompareCallback.class: de/crupp/hamsterdb/DuplicateCompareCallback.java
	$(JDK)/bin/javac de/crupp/hamsterdb/DuplicateCompareCallback.java

de/crupp/hamsterdb/Transaction.class: de/crupp/hamsterdb/Transaction.java
	$(JDK)/bin/javac de/crupp/hamsterdb/Transaction.java

clean:
	\rm -rf de/crupp/hamsterdb/*.class
	\rm -rf *.jar

javah:
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.CompareCallback
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Const
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Cursor
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.DatabaseException
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Database
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.DuplicateCompareCallback
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Environment
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.ErrorHandler
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.License
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Parameter
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.PrefixCompareCallback
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Transaction
	$(JDK)/bin/javah -d ../src de.crupp.hamsterdb.Version
