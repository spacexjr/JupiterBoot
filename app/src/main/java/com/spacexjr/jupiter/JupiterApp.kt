package com.spacexjr.jupiter

import android.app.Application
import com.spacexjr.jupiter.core.log.OperationLog

class JupiterApp : Application() {

    override fun onCreate() {
        super.onCreate()
        // Log global do processo; o ViewModel mantém o próprio canal para a UI.
        OperationLog(null).append(
            com.spacexjr.jupiter.core.log.LogLevel.INFO,
            com.spacexjr.jupiter.core.log.LogSource.APP,
            "Jupiter iniciado (Android ${android.os.Build.VERSION.RELEASE} · ${android.os.Build.MODEL})",
        )
    }
}
