CPP=g++
OFILES= crosswalk.o
LIBS= -lopencv_highgui -lopencv_core -lopencv_imgproc -lopencv_objdetect -lopencv_videoio -lopencv_video -lopencv_imgcodecs -lopencv_tracking

%.o : %.cpp
	$(CPP) -O2 -c -o $@ $<

all: crosswalk cardetector reader background flow tracking

crosswalk: crosswalk.o
	$(CPP) -o $@ $^ $(LIBS)

cardetector: cardetector.o
	$(CPP) -o $@ $^ $(LIBS)

reader: reader.o
	$(CPP) -o $@ $^ $(LIBS)

background: background.o
	$(CPP) -o $@ $^ $(LIBS)

flow: flow.o
	$(CPP) -o $@ $^ $(LIBS)

tracking: tracking.o
	$(CPP) -o $@ $^ $(LIBS)

