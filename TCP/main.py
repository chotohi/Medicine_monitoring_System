from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from tortoise.contrib.fastapi import register_tortoise

from models import MedInfo


app = FastAPI(title="药品仓储监测 API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 开发阶段允许跨域；生产环境应限制为实际网页域名
    allow_credentials=False,
    allow_methods=["GET"],
    allow_headers=["*"],
)


@app.get("/api/med")
async def get_all():
    """返回带状态码结构的全部药品数据。"""
    data = await MedInfo.all().values()
    return {"code": 0, "data": data}


@app.get("/meds")
async def get_all_meds():
    """返回网页展示所需的药品数组。"""
    return await MedInfo.all().values()


@app.get("/api/med/{name}")
async def get_one(name: str):
    """按药品名称查询一条状态。"""
    data = await MedInfo.filter(name=name).values().first()
    if not data:
        return {"code": 1, "msg": "not found"}
    return {"code": 0, "data": data}


register_tortoise(
    app,
    db_url="sqlite://medinfo.db",
    modules={"models": ["models"]},
    generate_schemas=True,
)
