package com.spacexjr.jupiter

import android.os.Bundle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.ComponentActivity
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.lifecycle.viewmodel.compose.viewModel
import com.spacexjr.jupiter.ui.JupiterRoot
import com.spacexjr.jupiter.ui.JupiterViewModel

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val vm: JupiterViewModel = viewModel()
            JupiterRoot(vm)
        }
    }
}
