from django.db import models

# {
#   "timestamp": "{{{PARTICLE_PUBLISHED_AT}}}",
#   "emergency": "{{{emergency}}}",
#   "latitude": "{{{latitude}}}",
#   "longitude": "{{{longitude}}}",
#   "accuracy": "{{{accuracy}}}"
# }

class request_logs(models.Model):
    timestamp=models.CharField(max_length=25)
    
    # emergency contains string one of 'medical' & 'police'
    emergency_type=models.CharField(max_length=7)

    # device id sending emergency message
    core_id=models.CharField(max_length=50)

    # latitude and longitude
    latitude = models.FloatField()
    longitude = models.FloatField()

    # accuracy
    accuracy = models.FloatField()

    def __str__(self):
        return self.emergency_type+" "+self.core_id