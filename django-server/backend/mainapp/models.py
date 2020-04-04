from django.db import models

class request_logs(models.Model):
    # string of format "%Y-%m-%d %H:%M:%S" eg. "2020-04-01 20:01:14", FORMAT = UTC
    timestamp=models.CharField(max_length=19, default='-')
    
    # emergency_ype contains string one of 'medical' & 'police'
    emergency_type=models.CharField(max_length=7, default='-')

    # core_id: if of sending device
    core_id = models.CharField(max_length=30, default='-')

    # latitude and longitude
    latitude = models.FloatField(default=-1.0)
    longitude = models.FloatField(default=-1.0)

    # accuracy
    accuracy = models.FloatField()

    # status: one of string: 'a' (active), 'w' (working), 'r' (resolved)
    status = models.CharField(max_length=1, default='a')

    def __str__(self):
        return self.timestamp + self.emergency_type + self.core_id