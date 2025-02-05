# Load Required Libraries -----------------------------------------------------
library(knitr)     # For generating dynamic reports
library(readr)     # For reading and writing CSV files
library(dplyr)     # For data manipulation
library(corrplot)  # For visualizing correlation matrices
library(ggplot2)   # For creating plots and visualizations
library(stargazer) # For creating regression tables
library(pander)    # For creating simple markdown-style tables


# Data Loading ---------------------------------------------------------------
# Set working directory (to load files from specific folder)
setwd("~/Dropbox/StatisticalConsultingClass/StrokeRiskCognitiveAbility/2017StudentWork")
setwd("/Users/ariana/Dropbox/StatisticalConsultingClass/2017Spring/StrokeRiskCognitiveAbility/2017StudentWork")

# Load phenotype and fMRI data
info <- read.csv("NKI_Phenotypic_Info_Stroke.csv")  # Phenotypic information
x <- read.csv("hrf_parameters.csv")  # fMRI parameters

# Convert 'Race' column to a factor variable and correct race categories
info$Race <- as.factor(info$Race)
levels(info$Race)[3] <- "AFRICAN AMERICAN"  # Rename third level of Race to 'AFRICAN AMERICAN'


# Data Cleaning --------------------------------------------------------------
# Filter out rows where Stroke equals 1 (i.e., exclude stroke cases)
info2 <- subset(info, Stroke != 1)

# Select relevant columns for analysis
info2 <- select(info2, Anonymized.ID, WASI.Full.Scale...4.Percentile.Rank, WIAT.Composite.Standard.Score,
                ANT.Test.Accuracy, Calculated.Age, Sex, Race, Volunteer.Highest.grade.completed.,
                Subject.SES.Score, Current.Smoker, Heart.Attack.Myocardial.Infarction,
                Corory.Artery.Disease..Clogged.Blocked.Heart.Arteries., Heart.Valve.Disease,
                High.Cholesterol, High.Blood.Pressure..Hypertension., Irregular.Heartbeat..Arrhythmia.,
                Diabetes.type.2, BMI, SUBSTANCE.INVOLVEMENT..SI..SCORE.ALCOHOL,
                Have.you.been.a.smoker.within.the.past.two.years.)

# Remove missing data
info2 <- na.omit(info2)

# Merge fMRI data with phenotypic data based on Anonymized.ID
info2 <- merge(x, info, by = "Anonymized.ID")

# Create a new variable for BMI categories (obese = 1 if BMI > 26)
info2$BMI_obese <- ifelse(info2$BMI > 26, 1, 0)

# Create a composite variable representing medical risk
info2$medical_risk_num <- (info2$Heart.Attack.Myocardial.Infarction +
                           info2$Corory.Artery.Disease..Clogged.Blocked.Heart.Arteries. +
                           info2$Heart.Valve.Disease + info2$High.Cholesterol +
                           info2$High.Blood.Pressure..Hypertension. + info2$Irregular.Heartbeat..Arrhythmia. +
                           info2$Diabetes.type.2 + info2$BMI_obese)

# Convert substance involvement score to numeric
info2$SUBSTANCE.INVOLVEMENT..SI..SCORE.ALCOHOL <- as.numeric(info2$SUBSTANCE.INVOLVEMENT..SI..SCORE.ALCOHOL)

# Categorize medical risk (0, 1, 2, 3+)
info2$medical_risk <- ifelse(info2$medical_risk_num == 0, "0",
                             ifelse(info2$medical_risk_num == 1, "1",
                                    ifelse(info2$medical_risk_num == 2, "2", "3+")))
info2$medical_risk <- factor(info2$medical_risk)


# Exploratory Data Analysis -------------------------------------------------
# Table of Race vs Sex distribution
tab_int <- table(info$Race, info$Sex)
pander(tab_int, style = "rmarkdown", page.break = FALSE)

# Histogram of medical risk distribution
ggplot(info2, aes(medical_risk_num)) +
  geom_histogram(binwidth = 1, fill = "blue", color = "white") +
  ggtitle("Histogram of Medical Risk") +
  theme(plot.title = element_text(hjust = 0.5))


# Regression Models --------------------------------------------------------
# Traditional WIAT Model: WIAT Composite Score ~ Age, Sex, Race, Education, SES
wiat_trad <- lm(WIAT.Composite.Standard.Score ~ Calculated.Age + Sex + Race + Volunteer.Highest.grade.completed. +
                  Subject.SES.Score, data = info2)

# Full WIAT Model with medical risk as an additional predictor
wiat_full2 <- lm(WIAT.Composite.Standard.Score ~ Calculated.Age + Sex + Race + Volunteer.Highest.grade.completed. +
                  Subject.SES.Score + info2$medical_risk_num, data = info2)

summary(wiat_full2)

# Extract residuals from the traditional WIAT model
info2$wiat_trad_resids <- wiat_trad$residuals

# Full WIAT model with all health-related variables (not just medical risk)
wiat_full3 <- lm(WIAT.Composite.Standard.Score ~ Calculated.Age + Sex + Race + Volunteer.Highest.grade.completed. +
                  Subject.SES.Score + info2$Heart.Attack.Myocardial.Infarction +
                  info2$Corory.Artery.Disease..Clogged.Blocked.Heart.Arteries. + info2$Heart.Valve.Disease +
                  info2$High.Cholesterol + Have.you.been.a.smoker.within.the.past.two.years. +
                  info2$High.Blood.Pressure..Hypertension. + info2$Irregular.Heartbeat..Arrhythmia. +
                  info2$Diabetes.type.2 + info2$BMI, data = info2)

summary(wiat_full3)

# Compare models using ANOVA
anova(wiat_trad, wiat_full3)

# Full WIAT Model including medical risk
wiat_full <- update(wiat_trad, . ~ . + medical_risk)

# WASI Models --------------------------------------------------------------
# Traditional WASI Model: WASI Full Scale Percentile Rank ~ Age, Sex, Race, Education, SES
wasi_trad <- lm(WASI.Full.Scale...4.Percentile.Rank ~ Calculated.Age + Sex + Race + Volunteer.Highest.grade.completed. +
                  Subject.SES.Score, data = info2)
summary(wasi_trad)

# Full WASI Model with medical risk as an additional predictor
wasi_full <- update(wasi_trad, . ~ . + medical_risk)

# Full WASI Model with all health-related variables
wasi_full2 <- lm(WASI.Full.Scale...4.Percentile.Rank ~ Calculated.Age + Sex + Race + Volunteer.Highest.grade.completed. +
                  Subject.SES.Score + info2$medical_risk_num, data = info2)
summary(wasi_full2)

# WASI model with all health-related variables (including smoking and cholesterol)
wasi_full3 <- lm(WASI.Full.Scale...4.Percentile.Rank ~ Calculated.Age + Sex + Race + Volunteer.Highest.grade.completed. +
                  Subject.SES.Score + info2$Heart.Attack.Myocardial.Infarction +
                  info2$Corory.Artery.Disease..Clogged.Blocked.Heart.Arteries. + info2$Heart.Valve.Disease +
                  info2$High.Cholesterol + info2$High.Blood.Pressure..Hypertension. +
                  info2$Irregular.Heartbeat..Arrhythmia. + info2$Diabetes.type.2 + info2$BMI +
                  info2$Have.you.been.a.smoker.within.the.past.two.years., data = info2)

summary(wasi_full3)

# Dependent variable interaction models (e.g., High Cholesterol and other covariates)
wasi_full3 <- lm(WASI.Full.Scale...4.Percentile.Rank ~ info2$High.Cholesterol, data = info2[info2$Calculated.Age > 59,])
summary(wasi_full3)

# Create plots to visualize relationships between depression, cholesterol, and cognitive ability
ggplot(z, aes(y = WASI, x = Total.Score, color = as.factor(HighCholesterol))) +
  ggtitle("Cognitive Ability, Depression, and Vascular Health \n in an Elderly Population") +
  geom_smooth(method = 'lm', fullrange = TRUE) +
  labs(x = "Geriatric Depression Scale", y = "WASI") +
  theme_bw() +
  scale_colour_manual(values = cbPalette)

# Correlation Plot for Depression and Cholesterol
corrplot.mixed(cor(depression, use = "complete.obs"))

# Create regression tables using Stargazer package
stargazer(wiat_trad, wiat_full, notes = c("F change=0.5945, p value=0.6188"),
          title = "Traditional and Full Linear Regression Models for WIAT Score (Academic Achievement)",
          dep.var.labels = "WIAT Score (Academic Achievement)", report = "vc*",
          intercept.bottom = FALSE, header = FALSE)

stargazer(wasi_trad, wasi_full, notes = c("F change=1.0959, p value=0.3505"),
          title = "Traditional and Full Linear Regression Models for WASI Score (Cognitive Ability)",
          dep.var.labels = "WASI Score (Cognitive Ability)", report = "vc*",
          intercept.bottom = FALSE, header = FALSE)

# Summary of residuals for WIAT and WASI
stargazer(wiat_resid, wasi_resid, title = "Linear Regression Models for WIAT and WASI Residuals",
          dep.var.caption = "Model (1) Residuals", dep.var.labels = c("WIAT Score (Academic Achievement)",
                                                                  "WASI Score (Cognitive Ability)"),
          report = "vc*", intercept.bottom = FALSE, header = FALSE)

# Correlation matrix and other appendices
corrplot(cor(info3[, c(2, 3, 5, 8, 9, 18, 21)], use = "complete.obs"), method = "square")

# Generate histograms for ANT Test Accuracy
ggplot(info2, aes(ANT.Test.Accuracy)) +
  geom_histogram(color = "white", fill = "blue") +
  ggtitle("Histogram of ANT Scores") +
  theme(plot.title = element_text(hjust = 0.5))

# Further interaction plots and statistical analysis
interaction.plot(info2$Sex, info2$Race, info2$WIAT.Composite.Standard.Score,
                 xlab = "Sex", trace.label = "Race", ylab = "WIAT Score",
                 main = "Interaction Plot between Race and Sex for WIAT",
                 col = c("red", "blue"), lwd = 3)

# Save results or append more models as needed

