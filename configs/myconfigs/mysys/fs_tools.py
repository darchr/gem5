
from m5.objects import IdeDisk, CowDiskImage, RawDiskImage

class CowDisk(IdeDisk):

    def __init__(self, filename):
        super(CowDisk, self).__init__()
        self.driveID = 'master'
        self.image = CowDiskImage(child=RawDiskImage(read_only=True),
                                  read_only=False)
        self.image.child.image_file = filename
