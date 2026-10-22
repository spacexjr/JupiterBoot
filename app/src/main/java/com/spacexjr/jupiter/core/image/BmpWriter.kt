package com.spacexjr.jupiter.core.image

import android.graphics.Bitmap
import android.graphics.Matrix
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import kotlin.math.max
import kotlin.math.min

/**
 * Exporta um Bitmap para BMP 24 bits, o unico formato que o renderizador do
 * JupiterBoot carrega hoje (src/jbm_menu.c, jbm_bg_load). O recorte e feito
 * antes da escala para preservar a proporcao de destino usada na tela.
 */
object BmpWriter {

    fun cropToBmp(
        source: Bitmap,
        zoom: Float,
        offsetX: Float,
        offsetY: Float,
        rotation: Int,
        targetWidth: Int,
        targetHeight: Int,
        out: File,
    ): Boolean {
        if (targetWidth <= 0 || targetHeight <= 0) return false
        val rotated = rotate(source, rotation)
        return try {
            val zoomSafe = zoom.coerceIn(1f, 5f)
            var cropW = (rotated.width / zoomSafe).toInt().coerceAtLeast(1)
            var cropH = (cropW.toFloat() * targetHeight / targetWidth).toInt().coerceAtLeast(1)
            if (cropH > rotated.height) {
                cropH = rotated.height
                cropW = (cropH.toFloat() * targetWidth / targetHeight).toInt().coerceAtLeast(1)
            }
            val maxX = (rotated.width - cropW).coerceAtLeast(0)
            val maxY = (rotated.height - cropH).coerceAtLeast(0)
            val cx = ((maxX / 2f) + offsetX.coerceIn(-1f, 1f) * (maxX / 2f)).toInt().coerceIn(0, maxX)
            val cy = ((maxY / 2f) + offsetY.coerceIn(-1f, 1f) * (maxY / 2f)).toInt().coerceIn(0, maxY)
            val scaled = Bitmap.createScaledBitmap(
                Bitmap.createBitmap(rotated, cx, cy, cropW, cropH),
                targetWidth,
                targetHeight,
                true,
            )
            out.parentFile?.mkdirs()
            writeBmp(scaled, out)
            if (scaled !== rotated) scaled.recycle()
            true
        } catch (_: Exception) {
            false
        } finally {
            if (rotated !== source) rotated.recycle()
        }
    }

    private fun rotate(source: Bitmap, rotation: Int): Bitmap {
        val deg = ((rotation % 360) + 360) % 360
        if (deg == 0) return source
        val matrix = Matrix().apply { postRotate(deg.toFloat()) }
        return Bitmap.createBitmap(source, 0, 0, source.width, source.height, matrix, true)
    }

    private fun writeBmp(bitmap: Bitmap, out: File) {
        val w = bitmap.width
        val h = bitmap.height
        val rowSize = ((w * 3 + 3) / 4) * 4
        val pixelBytes = rowSize * h
        val fileSize = 54 + pixelBytes
        BufferedOutputStream(FileOutputStream(out)).use { os ->
            fun le16(v: Int) {
                os.write(v and 0xFF)
                os.write((v ushr 8) and 0xFF)
            }
            fun le32(v: Int) {
                os.write(v and 0xFF)
                os.write((v ushr 8) and 0xFF)
                os.write((v ushr 16) and 0xFF)
                os.write((v ushr 24) and 0xFF)
            }
            os.write(byteArrayOf('B'.code.toByte(), 'M'.code.toByte()))
            le32(fileSize)
            le16(0)
            le16(0)
            le32(54)
            le32(40)
            le32(w)
            le32(h)
            le16(1)
            le16(24)
            le32(0)
            le32(pixelBytes)
            le32(2835)
            le32(2835)
            le32(0)
            le32(0)
            val row = ByteArray(rowSize)
            val pixels = IntArray(w)
            for (y in h - 1 downTo 0) {
                bitmap.getPixels(pixels, 0, w, 0, y, w, 1)
                var o = 0
                for (x in 0 until w) {
                    val c = pixels[x]
                    row[o++] = (c and 0xFF).toByte()
                    row[o++] = ((c ushr 8) and 0xFF).toByte()
                    row[o++] = ((c ushr 16) and 0xFF).toByte()
                }
                while (o < rowSize) row[o++] = 0
                os.write(row)
            }
        }
    }

    /** Dimensoes de destino sugeridas a partir da tela, limitadas ao BMP. */
    fun targetSize(screenWidth: Int, screenHeight: Int): Pair<Int, Int> {
        val w = max(240, min(screenWidth, 2160))
        val h = max(320, min(screenHeight, 3840))
        return w to h
    }
}
