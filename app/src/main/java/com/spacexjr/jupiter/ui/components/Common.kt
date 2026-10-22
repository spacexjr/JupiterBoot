package com.spacexjr.jupiter.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Error
import androidx.compose.material.icons.filled.Help
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Card
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

enum class StatusTone { OK, WARN, ERROR, INFO, UNKNOWN }

data class StatusVisual(val icon: ImageVector, val color: Color)

@Composable
fun toneVisual(tone: StatusTone): StatusVisual = when (tone) {
    StatusTone.OK -> StatusVisual(Icons.Filled.CheckCircle, MaterialTheme.colorScheme.primary)
    StatusTone.WARN -> StatusVisual(Icons.Filled.Warning, Color(0xFFB26A00))
    StatusTone.ERROR -> StatusVisual(Icons.Filled.Error, MaterialTheme.colorScheme.error)
    StatusTone.INFO -> StatusVisual(Icons.Filled.Info, MaterialTheme.colorScheme.tertiary)
    StatusTone.UNKNOWN -> StatusVisual(Icons.Filled.Help, MaterialTheme.colorScheme.outline)
}

/** Cartão de estado com ícone, título e valor. */
@Composable
fun StatusCard(
    title: String,
    value: String,
    tone: StatusTone,
    subtitle: String? = null,
    modifier: Modifier = Modifier,
    footer: (@Composable () -> Unit)? = null,
) {
    val visual = toneVisual(tone)
    Card(modifier = modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            Icon(
                imageVector = visual.icon,
                contentDescription = null,
                tint = visual.color,
                modifier = Modifier.size(28.dp),
            )
            Column(modifier = Modifier.fillMaxWidth()) {
                Text(
                    text = title,
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
                Text(
                    text = value,
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.SemiBold,
                )
                if (subtitle != null) {
                    Text(
                        text = subtitle,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
                footer?.invoke()
            }
        }
    }
}

/** Linha chave/valor de cartão de informação. */
@Composable
fun InfoRow(label: String, value: String, modifier: Modifier = Modifier) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.Medium,
        )
    }
}
