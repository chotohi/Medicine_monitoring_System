from tortoise import models, fields

class MedInfo(models.Model):
    name = fields.CharField(max_length=20, pk=True)
    humidity = fields.FloatField()
    temp = fields.FloatField()
    weight = fields.FloatField()

    class Meta:
        table = "med_info"
