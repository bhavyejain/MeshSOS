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
    emergency=models.CharField(max_length=50)

    # latitude and longitude
    latitude = models.FloatField()
    longitude = models.FloatField()

    # accuracy
    accuracy = models.FloatField()

    def __str__(self):
        return self.emergency