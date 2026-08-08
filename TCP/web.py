# -*- coding: utf-8 -*-
import os
from pathlib import Path

import httpx
from fastapi import FastAPI, HTTPException
from fastapi.responses import HTMLResponse, JSONResponse


app = FastAPI(title="药品仓储监测网页")
BASE_DIR = Path(__file__).resolve().parent
API_BASE_URL = os.getenv("API_BASE_URL", "http://127.0.0.1:8079").rstrip("/")


@app.get("/", response_class=HTMLResponse)
async def index():
    """返回监测展示页面。"""
    return (BASE_DIR / "static" / "index.html").read_text(encoding="utf-8")


@app.get("/api/meds")
async def proxy_meds():
    """代理内部数据 API，避免把数据库接口直接暴露到公网。"""
    try:
        async with httpx.AsyncClient(timeout=5.0) as client:
            response = await client.get(f"{API_BASE_URL}/meds")
            response.raise_for_status()
    except httpx.HTTPError as exc:
        raise HTTPException(status_code=502, detail="数据 API 暂不可用") from exc

    return JSONResponse(response.json())
